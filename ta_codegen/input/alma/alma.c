/* Arnaud Legoux Moving Average, created by Arnaud Legoux and Dimitris
 * Kouzis-Loukas: "ALMA; In search for the perfect Moving Average", 2009,
 * https://web.archive.org/web/20110904091012/www.arnaudlegoux.com/wp-content/uploads/2011/03/ALMA-Arnaud-Legoux-Moving-Average.pdf
 *
 * List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 MF,CC  First version (issue #475).
 */

int alma_lookback(int optInTimePeriod, double optInSigma, double optInOffset)
{
   (void)optInSigma;
   (void)optInOffset;
   return optInTimePeriod - 1;
}

TA_RetCode alma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInSigma,
   double optInOffset,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sum, s, twoSSq, d, norm;
   int lookbackTotal, outIdx, i, j, w, m;

   /* Filled once and read as a plain array. */
   CIRCBUF_PROLOG(weights,double,30);

   lookbackTotal = optInTimePeriod - 1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* A dot product seeded with 0.0 would turn a -0.0 input into +0.0. */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;
      i = startIdx;
      for( outIdx = 0; outIdx < (int)*outNBElement; outIdx++ )
         outReal[outIdx] = inReal[i++];
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( weights, double, optInTimePeriod );

   /* The expression order is the spec: it is what makes the output bitwise
    * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
    * in double (an int square overflows at large periods).
    */
   m = (int)floor( optInOffset * (double)lookbackTotal );
   s = (double)optInTimePeriod / optInSigma;
   twoSSq = 2.0 * s * s;
   norm = 0.0;
   for( j = 0; j < optInTimePeriod; j++ )
   {
      d = (double)(j - m);
      weights[j] = exp( -(d*d) / twoSSq );
      norm += weights[j];
   }
   for( j = 0; j < optInTimePeriod; j++ )
      weights[j] = weights[j] / norm;

   /* Weight 0 is the oldest bar. Each output is written after its window is
    * read, and the next window starts past it, so outReal may alias inReal.
    */
   outIdx = 0;
   i = startIdx;
   while( i <= endIdx )
   {
      sum = 0.0;
      w = 0;
      for( j = i - lookbackTotal; j <= i; j++ )
      {
         sum += weights[w] * inReal[j];
         w++;
      }
      outReal[outIdx] = sum;
      outIdx++;
      i++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(weights);

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={BATCH,ALL_LANGUAGES} */
TA_RetCode alma_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInSigma,
   double optInOffset,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sum, s, twoSSq, d, norm, wj, s0, s1, s2, s3;
   int lookbackTotal, outIdx, i, j, w, m, b;

   /* Filled once and read as a plain array. */
   CIRCBUF_PROLOG(weights,double,30);

   lookbackTotal = optInTimePeriod - 1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* A dot product seeded with 0.0 would turn a -0.0 input into +0.0. */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;
      i = startIdx;
      for( outIdx = 0; outIdx < (int)*outNBElement; outIdx++ )
         outReal[outIdx] = inReal[i++];
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( weights, double, optInTimePeriod );

   /* The expression order is the spec: it is what makes the output bitwise
    * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
    * in double (an int square overflows at large periods).
    */
   m = (int)floor( optInOffset * (double)lookbackTotal );
   s = (double)optInTimePeriod / optInSigma;
   twoSSq = 2.0 * s * s;
   norm = 0.0;
   for( j = 0; j < optInTimePeriod; j++ )
   {
      d = (double)(j - m);
      weights[j] = exp( -(d*d) / twoSSq );
      norm += weights[j];
   }
   for( j = 0; j < optInTimePeriod; j++ )
      weights[j] = weights[j] / norm;

   /* 4 bars per pass, each with its own accumulator summed in the same
    * order as the base: the same bits, with 4 independent add chains in
    * flight instead of one. Every window of a pass is read before any of its
    * outputs is written, and the next pass reads past them, so outReal may
    * alias inReal.
    */
   outIdx = 0;
   i = startIdx;
   while( i + 3 <= endIdx )
   {
      s0 = 0.0;
      s1 = 0.0;
      s2 = 0.0;
      s3 = 0.0;
      b = i - lookbackTotal;
      for( j = 0; j < optInTimePeriod; j++ )
      {
         wj = weights[j];
         s0 += wj * inReal[b];
         s1 += wj * inReal[b+1];
         s2 += wj * inReal[b+2];
         s3 += wj * inReal[b+3];
         b++;
      }
      outReal[outIdx] = s0;
      outReal[outIdx+1] = s1;
      outReal[outIdx+2] = s2;
      outReal[outIdx+3] = s3;
      outIdx += 4;
      i += 4;
   }

   while( i <= endIdx )
   {
      sum = 0.0;
      w = 0;
      for( j = i - lookbackTotal; j <= i; j++ )
      {
         sum += weights[w] * inReal[j];
         w++;
      }
      outReal[outIdx] = sum;
      outIdx++;
      i++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(weights);

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={BATCH,RUST} */
TA_RetCode alma_ALT2(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInSigma,
   double optInOffset,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sum, s, twoSSq, d, norm, wj, s0, s1, s2, s3;
   int lookbackTotal, outIdx, i, j, w, m;

   /* Filled once and read as a plain array. */
   CIRCBUF_PROLOG(weights,double,30);

   lookbackTotal = optInTimePeriod - 1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* A dot product seeded with 0.0 would turn a -0.0 input into +0.0. */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;
      i = startIdx;
      for( outIdx = 0; outIdx < (int)*outNBElement; outIdx++ )
         outReal[outIdx] = inReal[i++];
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( weights, double, optInTimePeriod );

   /* The expression order is the spec: it is what makes the output bitwise
    * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
    * in double (an int square overflows at large periods).
    */
   m = (int)floor( optInOffset * (double)lookbackTotal );
   s = (double)optInTimePeriod / optInSigma;
   twoSSq = 2.0 * s * s;
   norm = 0.0;
   for( j = 0; j < optInTimePeriod; j++ )
   {
      d = (double)(j - m);
      weights[j] = exp( -(d*d) / twoSSq );
      norm += weights[j];
   }
   for( j = 0; j < optInTimePeriod; j++ )
      weights[j] = weights[j] / norm;

   /* ALT1's passes with j running over the window itself: Rust reads only this
    * form through a slice, without a bounds check per term, and the same form
    * costs Java up to 2x at long periods.
    */
   outIdx = 0;
   i = startIdx;
   while( i + 3 <= endIdx )
   {
      s0 = 0.0;
      s1 = 0.0;
      s2 = 0.0;
      s3 = 0.0;
      w = 0;
      for( j = i - lookbackTotal; j <= i; j++ )
      {
         wj = weights[w];
         s0 += wj * inReal[j];
         s1 += wj * inReal[j+1];
         s2 += wj * inReal[j+2];
         s3 += wj * inReal[j+3];
         w++;
      }
      outReal[outIdx] = s0;
      outReal[outIdx+1] = s1;
      outReal[outIdx+2] = s2;
      outReal[outIdx+3] = s3;
      outIdx += 4;
      i += 4;
   }

   while( i <= endIdx )
   {
      sum = 0.0;
      w = 0;
      for( j = i - lookbackTotal; j <= i; j++ )
      {
         sum += weights[w] * inReal[j];
         w++;
      }
      outReal[outIdx] = sum;
      outIdx++;
      i++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(weights);

   return TA_SUCCESS;
}
