/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AF       Alexander Trufanov (github @trufanov-nok)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  031202 MF     Template creation.
 *  052603 MF     Port to managed C++. Change to use CIRCBUF macros.
 *  061704 MF     Lower limit for period to 2, and correct algorithm
 *                to avoid cummulative error when value are close to
 *                the floating point epsilon.
 *  070626 AF,CC  Guard the final division with TA_IS_ZERO instead of an exact
 *                "!= 0.0" check: identical prices over the period leave
 *                sub-epsilon residue that the exact check divided into a
 *                spurious value (issue #7 / SF bug #107). Now returns 0.0.
 */

int cci_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode cci(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double tempReal, tempReal2, theAverage, lastValue;
   int i, j, outIdx, lookbackTotal;

   /* This ptr will points on a circular buffer of
    * at least "optInTimePeriod" element.
    */
   CIRCBUF_PROLOG(circBuffer,double,30);

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Allocate a circular buffer equal to the requested
    * period.
    */
   CIRCBUF_INIT( circBuffer, double, optInTimePeriod );

   /* Do the MA calculation using tight loops. */

   /* Add-up the initial period, except for the last value.
    * Fill up the circular buffer at the same time.
    */
   i=startIdx-lookbackTotal;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx )
      {
         circBuffer[circBuffer_Idx] = (inHigh[i]+inLow[i]+inClose[i])/3;
         i++;
         CIRCBUF_NEXT(circBuffer);
      }
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the inReal and
    * outReal to be the same buffer.
    */
   outIdx = 0;
   do
   {
      lastValue = (inHigh[i]+inLow[i]+inClose[i])/3;
      circBuffer[circBuffer_Idx] = lastValue;

      /* Calculate the average for the whole period. */
      theAverage = 0;
      for( j=0; j < optInTimePeriod; j++ )
         theAverage += circBuffer[j];
      theAverage /= optInTimePeriod;

      /* Do the summation of the ABS(TypePrice-average)
       * for the whole period.
       */
      tempReal2 = 0;
      for( j=0; j < optInTimePeriod; j++ )
         tempReal2 += fabs(circBuffer[j]-theAverage);

      /* And finally, the CCI... */
      tempReal = lastValue-theAverage;

      if( !TA_IS_ZERO(tempReal) && !TA_IS_ZERO(tempReal2) )
      {
         outReal[outIdx++] = tempReal/(0.015*(tempReal2/optInTimePeriod));
      }
      else
         outReal[outIdx++] = 0.0;

      /* Move forward the circular buffer indexes. */
      CIRCBUF_NEXT(circBuffer);

      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   /* Free the circular buffer if it was dynamically allocated. */
   CIRCBUF_DESTROY(circBuffer);

   return TA_SUCCESS;
}
