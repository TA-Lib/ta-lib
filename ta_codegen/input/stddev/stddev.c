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
 *  082326 MF,CC #243 the sqrt trap moves to var's scale-relative floor.
 */

int stddev_lookback(int optInTimePeriod, double optInNbDev)
{
   /* Lookback is driven by the variance. */
   return var_lookback( optInTimePeriod, optInNbDev );
}

TA_RetCode stddev(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDev,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   TA_RetCode retCode;

   /* Nothing to produce: the range is shorter than the lookback. Return before
    * touching anything.
    *
    * Same shape as the guard in apo and bbands: the variance below runs on the
    * same range and its lookback IS stddev's, so it declines and yields 0,0
    * without reading. Observably identical, but it makes "a range shorter than
    * the lookback reads nothing" true of stddev itself rather than only of var.
    * Pinned by the zero-length no-I/O probe over every guarded core.
    */
   if( stddev_lookback( optInTimePeriod, optInNbDev ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Calculate the variance. */
   retCode = var( startIdx, endIdx,
      inReal, optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );

   if( retCode != TA_SUCCESS )
      return retCode;

   /* Calculate the square root of each variance, this
    * is the standard deviation.
    *
    * Multiply also by the ratio specified.
    *
    * Unconditional. var owns the dead-zone and owns the sign: it returns a
    * non-negative variance, already floored to exactly 0 on any window whose
    * re-anchored spread sat under its own rounding noise (var.c). What used to
    * stand here instead - zero the output wherever the variance fell under
    * TA_EPSILON - compared a SQUARED quantity to a fixed 1e-14, which is a cliff
    * at a price level rather than a noise floor: a $100.00 instrument quoted in
    * 1e-8 ticks has a variance around 1e-16 and came back as exactly 0 on every
    * bar, with TA_SUCCESS and nothing to say it had been suppressed (#243).
    * Dropping it also leaves a pure map, which the branch had kept sqrt out of.
    */
   if( optInNbDev != 1.0 )
   {
      for( i=0; i < (int)*outNBElement; i++ )
         outReal[i] = sqrt(outReal[i]) * optInNbDev;
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
         outReal[i] = sqrt(outReal[i]);
   }

   return TA_SUCCESS;
}
