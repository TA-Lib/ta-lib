/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#200).
 *
 */

int wad_lookback(void)
{
   /* The first bar has no previous close, so it accumulates nothing and the
    * line starts at 0.0 -- the same convention as the other four cumulative
    * lines in the tree: OBV, AD, NVI and PVI all return 0 here and emit a
    * seed value at startIdx. Tulip's ti_wad_start() returns 1 instead, so its
    * series is this one without the leading zero.
    */
   return 0;
}

TA_RetCode wad(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sum, prevClose, close, trueExtreme;
   int i, outIdx;

   /* Williams' Accumulation/Distribution, in the form Steven Achelis
    * published (Technical Analysis from A to Z, 2nd ed., p.368) and the form
    * every modern vendor ships: each bar's close is measured against the TRUE
    * range extreme -- the previous close when it lies outside today's bar --
    * and the results accumulate.
    *
    *    TRH = max( prevClose, high )      TRL = min( prevClose, low )
    *    AD  = close - TRL   if close > prevClose
    *        = close - TRH   if close < prevClose
    *        = 0             if close == prevClose
    *    WAD = running sum of AD
    *
    * NO VOLUME IS CONSUMED, despite the name and despite the group this is
    * filed under. Larry Williams' original multiplies the move by volume;
    * Achelis' modification drops it, the industry attached Williams' name to
    * the modification anyway, and Tulip, pandas-ta-classic, cTrader, TC2000,
    * WealthCharts and MultiCharts all ship the no-volume form. Shipping the
    * volume form under this name would surprise every user, so this is the
    * one place the usual "the original author wins" rule is set aside. The
    * volume-weighted series is a different indicator.
    *
    * The three-way branch is written with plain > and < rather than any
    * epsilon: the flat arm must fire on exactly-equal consecutive closes and
    * on nothing else, which also keeps -0.0 and NaN behaviour identical
    * across the C, Rust, Java and .NET backends.
    *
    * prevClose is carried in a scalar, so outReal may alias any input: every
    * read of bar i happens before the store at outIdx <= i, and no earlier
    * bar is ever re-read.
    */

   sum = 0.0;
   outIdx = 0;

   /* The first bar of the requested range is measured against itself, i.e. it
    * contributes exactly 0.0. The accumulator therefore restarts wherever the
    * caller starts, which is why this function is flagged path_dependent.
    */
   prevClose = inClose[startIdx];

   for( i=startIdx; i <= endIdx; i++ )
   {
      close = inClose[i];

      if( close > prevClose )
      {
         trueExtreme = inLow[i];
         if( prevClose < trueExtreme )
            trueExtreme = prevClose;
         sum += close - trueExtreme;
      }
      else if( close < prevClose )
      {
         trueExtreme = inHigh[i];
         if( prevClose > trueExtreme )
            trueExtreme = prevClose;
         sum += close - trueExtreme;
      }

      outReal[outIdx] = sum;
      outIdx = outIdx + 1;
      prevClose = close;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
