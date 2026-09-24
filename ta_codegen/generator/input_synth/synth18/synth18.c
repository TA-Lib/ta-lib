/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  092426 MF,CC Creation (synthetic gate: initializers read before a top-level store, #438)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth18.md — one copy, so there is one thing to keep true.
 */

int synth18_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode synth18(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double cap = 1000.0;
   double weight = 2.0;
   double scale = 3.0;
   double offset = scale;
   double bias = 7.0;
   double first, ramp;
   int lookbackTotal, outIdx, i;

   lookbackTotal = optInTimePeriod-1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   first = inReal[startIdx-lookbackTotal];
   if( first > cap )
      cap = first;

   ramp = 0.0;
   i = startIdx-lookbackTotal;
   while( i < startIdx )
   {
      ramp += weight;
      i++;
   }
   weight = 0.5;

   scale = 0.25;
   bias += scale;

   outIdx = 0;
   i = startIdx;
   while( i <= endIdx )
   {
      outReal[outIdx++] = (inReal[i] - cap + ramp + offset + bias) * weight;
      i++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
