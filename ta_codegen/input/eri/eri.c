/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #361).
 */

int eri_lookback(int optInTimePeriod)
{
   /* Exactly the EMA of close underneath: its lookback, unstable period
    * included, is this function's lookback.
    */
   return ema_lookback( optInTimePeriod );
}

TA_RetCode eri(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx,
   int *outNBElement,
   double outBullPower[],
   double outBearPower[])
{
   int outIdx, today, lookbackTotal, i;
   double prevMA, tempReal, k;
   double tempHT, tempLT;

   /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
    * the bar's extremes sit from one shared EMA of close.
    *
    *   Bull Power = High - EMA(Close, n)
    *   Bear Power = Low  - EMA(Close, n)
    *
    * One fused loop, not ema() + a combine map: a composed form cannot
    * stream (raw bar inputs are outside check_map_step's provenance), which
    * is the same reason ACCBANDS is fused. The EMA is ema.c's DEFAULT arm
    * op for op -- sequential seed sum from 0.0 then one divide, the
    * unstable-period warm-up consumed bar by bar -- so the differential
    * against shipped TA_EMA holds bitwise. No compatibility branch: the
    * Metastock arm is unreachable from three of the four backends, and a
    * new function honouring it would make C diverge from them (EFI/SMI
    * precedent).
    *
    * No division in the per-bar map: no 0/0, no NaN path (#112 by
    * construction). Bull >= Bear on every bar since high >= low.
    */

   lookbackTotal = eri_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
    * exists there. At n == 1 the recursion below is fl(fl(x-prev)+prev),
    * which returns x only while consecutive closes stay within a factor of
    * two (Sterbenz), so without this arm `High - TA_EMA(Close, 1)` is not
    * what this function returns. The unstable period still delays the first
    * output, through the shared lookback above.
    */
   if( optInTimePeriod == 1 )
   {
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx )
      {
         tempHT   = inHigh[today];
         tempLT   = inLow[today];
         tempReal = inClose[today];
         outBullPower[outIdx] = tempHT - tempReal;
         outBearPower[outIdx] = tempLT - tempReal;
         outIdx++;
         today++;
      }
      *outBegIdx    = startIdx;
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   k = 2.0 / ( (double)optInTimePeriod + 1.0 );

   /* Seed: ema.c's DEFAULT arm, op for op. */
   today = startIdx - lookbackTotal;
   i = optInTimePeriod;
   tempReal = 0.0;
   while( i-- > 0 )
      tempReal += inClose[today++];
   prevMA = tempReal / optInTimePeriod;

   /* The warm-up also consumes the EMA unstable period. */
   while( today <= startIdx )
      prevMA = ((inClose[today++]-prevMA)*k) + prevMA;

   /* prevMA is the EMA at bar startIdx; today == startIdx + 1. Load the
    * extremes into temps BEFORE writing either output: with two outputs
    * over three inputs the caller may alias any pair, and the second write
    * must not read a clobbered bar.
    */
   tempHT = inHigh[startIdx];
   tempLT = inLow[startIdx];
   outBullPower[0] = tempHT - prevMA;
   outBearPower[0] = tempLT - prevMA;
   outIdx = 1;

   while( today <= endIdx )
   {
      prevMA = ((inClose[today]-prevMA)*k) + prevMA;
      tempHT = inHigh[today];
      tempLT = inLow[today];
      outBullPower[outIdx] = tempHT - prevMA;
      outBearPower[outIdx] = tempLT - prevMA;
      outIdx++;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
