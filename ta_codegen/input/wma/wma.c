/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  082426 MF,CC Fix #255. Re-anchor the running sums: every 32*period bars,
 *               and on the bar a large value leaves the window.
 *
 */

int wma_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode wma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int inIdx, outIdx, i, trailingIdx;
   int j, rw, lookbackWin, barsSinceReseed;
   double periodSum, periodSub, tempReal, trailingValue, divider;
   int lookbackTotal;

   lookbackTotal = optInTimePeriod-1;

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

   /* To make the rest more efficient, handle exception
    * case where the user is asking for a period of '1'.
    * In that case outputs equals inputs for the requested
    * range.
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;

      /* Element loop, not a block copy: the C single-precision variant reads a
       * float array, so a double-sized byte copy would reinterpret and
       * over-read it (#137). Forward order keeps the in-place case correct (#94).
       */
      inIdx = startIdx;
      for( i = 0; i < (int)*outNBElement; i++ )
         outReal[i] = inReal[inIdx++];

      return TA_SUCCESS;
   }

   /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
    * int product n*(n+1) overflows int32 at n>=46341 (#142).
    */
   divider = (double)optInTimePeriod*(optInTimePeriod+1)/2.0;

   /* The algo used here use a very basic property of
    * multiplication/addition: (x*2) = x+x
    *
    * As an example, a 3 period weighted can be
    * interpreted in two way:
    *  (x1*1)+(x2*2)+(x3*3)
    *      OR
    *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
    *
    * When you move forward in the time serie
    * you can quickly adjust the periodSum for the
    * period by substracting:
    *   x1+x2+x3 (This is the periodSub)
    * Making the new periodSum equals to:
    *   x2+x3+x3
    *
    * You can then add the new price bar
    * which is x4+x4+x4 giving:
    *   x2+x3+x3+x4+x4+x4
    *
    * At this point one iteration is completed and you can
    * see that we are back to the step 1 of this example.
    *
    * Why making it so un-intuitive? The number of memory
    * access and floating point operations are kept to a
    * minimum with this algo.
    */
   outIdx      = 0;
   trailingIdx = startIdx - lookbackTotal;

   /* Evaluate the initial periodSum/periodSub and trailingValue. */
   lookbackWin = optInTimePeriod - 1;
   periodSum = periodSub = (double)0.0;
   inIdx=trailingIdx;
   i = 1;
   while( inIdx < startIdx )
   {
      tempReal = inReal[inIdx];
      inIdx++;
      periodSub += tempReal;
      periodSum += tempReal*i;
      i++;
   }
   barsSinceReseed = 8 * optInTimePeriod;
   trailingValue = 0.0;

   /* Tight loop for the requested range. */
   while( inIdx <= endIdx )
   {
      /* Add the current price bar to the sum
       * who are carried through the iterations.
       */
      tempReal = inReal[inIdx];
      periodSub += tempReal;
      periodSub -= trailingValue;
      periodSum += tempReal*optInTimePeriod;

      /* Re-anchor: rebuild both totals from the window itself.
       *
       * periodSum and periodSub were running totals that were never
       * recomputed, so each bar's rounding joined a residue no later bar
       * could subtract, and its size was set by the largest value the totals
       * had ever held rather than by the current window. That is the defect
       * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
       * below is the same weight-shifting identity as that family's
       * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
       * it and TA_SMA, whose output lives at its own sum's scale, does not.
       * Measured before the fix: worst range disagreement 1.41e-08 at 200000
       * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
       * closes or ~1000 with one large print. After: 1.79e-12, flat in call
       * length.
       *
       * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
       * LINEARREG family also carries an OUTLIER trigger (rebuild when the
       * departing value outweighs the window) because for a slope the
       * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
       * that position: its weights are bounded by `period` and its divider is
       * period*(period+1)/2, which dilutes the residue enough that the
       * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
       * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
       * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
       * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
       * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
       * shorter interval buys most of the accuracy for ~1.1x instead.
       *
       * The rebuild walks the window OLDEST FIRST with the weight counting UP
       * from 1 -- the priming scan's own order and weighting -- so a
       * re-anchored bar is bit-identical to the same bar computed by a call
       * that started there. That identity is what the range-stability
       * contract measures, and what test_wma.c W2/W3 assert.
       *
       * The loop start is written INLINE rather than through a `windowStart`
       * local: only that form is recognised as a rescan window, which is what
       * keeps this on the stream classifier's primary path. See
       * docs/ta_codegen_input_code.md.
       *
       * Reading the window is safe when outReal aliases inReal: the outputs
       * written so far occupy [0, outIdx-1], and the window starts at
       * startIdx-lookbackTotal+outIdx, which is >= outIdx.
       */
      barsSinceReseed--;
      if( barsSinceReseed <= 0 )
      {
         barsSinceReseed = 8 * optInTimePeriod;
         periodSub = (double)0.0;
         periodSum = (double)0.0;
         rw = 1;
         for( j = inIdx - lookbackWin; j <= inIdx; j++ )
         {
            tempReal = inReal[j];
            periodSub += tempReal;
            periodSum += tempReal*rw;
            rw++;
         }
      }

      /* Save the trailing value for being substract at
       * the next iteration.
       * (must be saved here just in case outReal and
       *  inReal are the same buffer).
       */
      trailingValue = inReal[trailingIdx];
      trailingIdx++;

      /* Calculate the WMA for this price bar. */
      outReal[outIdx++] = periodSum / divider;

      /* Prepare the periodSum for the next iteration. */
      periodSum -= periodSub;
      inIdx++;
   }

   /* Set output limits. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
