/* List of contributors:
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
 *  092526 MF,CC  Initial version (#450).
 *  092626 MF,CC  O(1) per bar over exact sums (#453).
 */

int cg_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode cg(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int today, outIdx, lookbackTotal, trailingIdx, j, k, limbs, fits;
   int maxAt, failAt, stickyBars, fit2Mid, fit2Lo, fit3Mid;
   double num, den, value, periodDouble, flatValue, weightTotal;
   double width2, width3, ylim2, ylim3, head2, head3;
   double scale, width, invWidth, widthSq, invWidthSq, ylim, maxAbs, half;
   double scale2Mid, scale2Lo, scale3Mid, scale3Lo;
   double x, y, a, b, c, q, t;
   double denA, denB, denC, numA, numB, numC;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = cg_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   periodDouble = (double)optInTimePeriod;
   flatValue = -(periodDouble + 1.0) * 0.5;

   /* Each window value is held as y = x*scale, an integer, split into 2 or 3
    * integer-valued limbs of `width`. The bounds keep every limb of both the
    * weighted and the plain sum below 2^53 while the total weight is at most
    * 2^tb, so every add and subtract below is exact, and every product is by a
    * power of two or of two integers whose result is below 2^53. That makes
    * each output depend on its window alone, whatever scale or limb count the
    * call reached it with. Keep it that way: a rounded operation anywhere in
    * the limb arithmetic makes the output depend on the start index.
    *
    * 2 limbs: width 2^(53-tb), |y| < 2^(104-2tb).
    * 3 limbs: width min(2^(53-tb), 2^26), |y| < 2^(51-tb) * width^2.
    *
    * Keep every product out of an addition in the same expression: each
    * product is exact, so fusing would change no value, only cost a call
    * where FMA is not inlined.
    */
   weightTotal = 1.0;
   while( weightTotal < periodDouble * (periodDouble + 1.0) * 0.5 )
      weightTotal *= 2.0;
   width2 = 9007199254740992.0 / weightTotal;
   ylim2 = 0.25 * width2 * width2;
   width3 = width2;
   if( width3 > 67108864.0 )
      width3 = 67108864.0;
   ylim3 = 0.25 * width2 * width3 * width3;

   /* A scale below the largest that fits leaves room for the window's
    * magnitude to grow before a rebuild, split evenly with the room left for
    * finer values.
    */
   head2 = 1.0;
   while( head2 * head2 * 9007199254740992.0 < ylim2 )
      head2 *= 2.0;
   head3 = 1.0;
   while( head3 * head3 * 9007199254740992.0 < ylim3 )
      head3 *= 2.0;

   limbs = 0;
   stickyBars = 0;
   scale = 1.0;
   width = 1.0;
   invWidth = 1.0;
   widthSq = 1.0;
   invWidthSq = 1.0;
   ylim = 0.0;
   half = 1.0;
   denA = 0.0;
   denB = 0.0;
   denC = 0.0;
   numA = 0.0;
   numB = 0.0;
   numC = 0.0;
   num = 0.0;
   den = 0.0;

   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx - lookbackTotal;
   while( today <= endIdx )
   {
      /* Between bars the sums hold the window less its oldest value. */
      fits = 0;
      x = inReal[today];
      y = x * scale;
      if( limbs == 2 )
      {
         if( fabs( y ) < ylim && (y != 0.0 || x == 0.0) )
         {
            t = y * invWidth;
            a = (t + 6755399441055744.0) - 6755399441055744.0;
            t = a * width;
            c = y - t;
            if( (c + 6755399441055744.0) - 6755399441055744.0 == c )
            {
               denA += a;
               denC += c;
               numA += denA;
               numC += denC;
               fits = 1;
            }
         }
      }
      else if( limbs == 3 )
      {
         if( fabs( y ) < ylim && (y != 0.0 || x == 0.0) )
         {
            t = y * invWidthSq;
            a = (t + 6755399441055744.0) - 6755399441055744.0;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = (t + 6755399441055744.0) - 6755399441055744.0;
            t = b * width;
            c = q - t;
            if( (c + 6755399441055744.0) - 6755399441055744.0 == c )
            {
               denA += a;
               denB += b;
               denC += c;
               numA += denA;
               numB += denB;
               numC += denC;
               fits = 1;
            }
         }
      }

      if( fits == 0 && limbs == 0 && stickyBars > 0 )
      {
         /* Both witnesses of the last failed fit are still in the window, so
          * it cannot fit either: its largest value can only be larger, which
          * only coarsens the scale the failing value already missed.
          */
         stickyBars--;
         num = 0.0;
         den = 0.0;
         for( j = today - lookbackTotal; j <= today; j++ )
         {
            den += inReal[j];
            num += den;
         }
      }
      else if( fits == 0 )
      {
         /* The incoming value does not fit the scale: pick one from the
          * window alone.
          */
         maxAbs = 0.0;
         maxAt = 0;
         k = 0;
         for( j = today - lookbackTotal; j <= today; j++ )
         {
            x = fabs( inReal[j] );
            if( x >= maxAbs )
            {
               maxAbs = x;
               maxAt = k;
            }
            k++;
         }

         /* The largest power of two not above the window's largest
          * magnitude. The search may start from any finite power of two, so
          * it starts from the last window's; keep it uncapped, or a start far
          * from the answer stops short and the fit then depends on the
          * previous window. A window with an infinite magnitude fails the fit
          * at any scale, so it keeps the last one.
          */
         if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 )
         {
            while( half * 65536.0 <= maxAbs )
               half *= 65536.0;
            while( half * 2.0 <= maxAbs )
               half *= 2.0;
            while( half > maxAbs * 65536.0 )
               half *= 0.0000152587890625;
            while( half > maxAbs )
               half *= 0.5;
         }

         /* The largest scale each limb count allows, and the same with
          * headroom, at most 2^1022 so that the scale itself is finite. The
          * largest 3-limb scale is the most permissive there is: the window
          * fits some scale only if every value is an integer at that one. A
          * scale below 1 can round a small value to 0, which is not a fit.
          */
         scale2Lo = ylim2 * 0.5 / half;
         if( scale2Lo > 4.49423283715579e307 )
            scale2Lo = 4.49423283715579e307;
         scale2Mid = scale2Lo / head2;
         scale3Lo = ylim3 * 0.5 / half;
         if( scale3Lo > 4.49423283715579e307 )
            scale3Lo = 4.49423283715579e307;
         scale3Mid = scale3Lo / head3;

         /* The same pass sums the window as it stands, oldest first, for when
          * nothing fits: non-finite values, bits below 2^-1022, or too wide a
          * span of magnitudes.
          */
         fit2Mid = 1;
         fit2Lo = 1;
         fit3Mid = 1;
         failAt = -1;
         num = 0.0;
         den = 0.0;
         k = 0;
         for( j = today - lookbackTotal; j <= today; j++ )
         {
            x = inReal[j];
            den += x;
            num += den;
            y = fabs( x * scale2Mid );
            if( !(y < ylim2) || (y < 4503599627370496.0 && (y + 4503599627370496.0) - 4503599627370496.0 != y)
               || (y == 0.0 && x != 0.0) )
            fit2Mid = 0;
            y = fabs( x * scale2Lo );
            if( !(y < ylim2) || (y < 4503599627370496.0 && (y + 4503599627370496.0) - 4503599627370496.0 != y)
               || (y == 0.0 && x != 0.0) )
            fit2Lo = 0;
            y = fabs( x * scale3Mid );
            if( !(y < ylim3) || (y < 4503599627370496.0 && (y + 4503599627370496.0) - 4503599627370496.0 != y)
               || (y == 0.0 && x != 0.0) )
            fit3Mid = 0;
            y = fabs( x * scale3Lo );
            if( !(y < ylim3) || (y < 4503599627370496.0 && (y + 4503599627370496.0) - 4503599627370496.0 != y)
               || (y == 0.0 && x != 0.0) )
            failAt = k;
            k++;
         }

         limbs = 0;
         if( failAt >= 0 )
         {
            if( failAt < maxAt )
               stickyBars = failAt;
            else
               stickyBars = maxAt;
         }
         else
         {
            if( fit2Mid == 1 || fit2Lo == 1 )
            {
               limbs = 2;
               width = width2;
               ylim = ylim2;
               if( fit2Mid == 1 )
                  scale = scale2Mid;
               else
                  scale = scale2Lo;
            }
            else
            {
               limbs = 3;
               width = width3;
               ylim = ylim3;
               if( fit3Mid == 1 )
                  scale = scale3Mid;
               else
                  scale = scale3Lo;
            }
            invWidth = 1.0 / width;
            widthSq = width * width;
            invWidthSq = invWidth * invWidth;

            denA = 0.0;
            denB = 0.0;
            denC = 0.0;
            numA = 0.0;
            numB = 0.0;
            numC = 0.0;
            if( limbs == 2 )
            {
               for( j = today - lookbackTotal; j <= today; j++ )
               {
                  y = inReal[j] * scale;
                  t = y * invWidth;
                  a = (t + 6755399441055744.0) - 6755399441055744.0;
                  t = a * width;
                  c = y - t;
                  denA += a;
                  denC += c;
                  numA += denA;
                  numC += denC;
               }
            }
            else
            {
               for( j = today - lookbackTotal; j <= today; j++ )
               {
                  y = inReal[j] * scale;
                  t = y * invWidthSq;
                  a = (t + 6755399441055744.0) - 6755399441055744.0;
                  t = a * widthSq;
                  q = y - t;
                  t = q * invWidth;
                  b = (t + 6755399441055744.0) - 6755399441055744.0;
                  t = b * width;
                  c = q - t;
                  denA += a;
                  denB += b;
                  denC += c;
                  numA += denA;
                  numB += denB;
                  numC += denC;
               }
            }
         }
      }

      /* One rounding each: the limbs are carried into range first, so the
       * last add sees two exact values.
       */
      if( limbs == 2 )
      {
         t = denA * width;
         den = t + denC;
         t = numA * width;
         num = t + numC;
      }
      else if( limbs == 3 )
      {
         t = denC * invWidth;
         q = (t + 6755399441055744.0) - 6755399441055744.0;
         b = denB + q;
         t = q * width;
         c = denC - t;
         t = b * invWidth;
         q = (t + 6755399441055744.0) - 6755399441055744.0;
         t = q * width;
         b = b - t;
         t = b * width;
         c = t + c;
         t = denA + q;
         t = t * widthSq;
         den = t + c;
         t = numC * invWidth;
         q = (t + 6755399441055744.0) - 6755399441055744.0;
         b = numB + q;
         t = q * width;
         c = numC - t;
         t = b * invWidth;
         q = (t + 6755399441055744.0) - 6755399441055744.0;
         t = q * width;
         b = b - t;
         t = b * width;
         c = t + c;
         t = numA + q;
         t = t * widthSq;
         num = t + c;
      }

      /* The denominator is a signed sum, so only an exact zero is degenerate.
       * It is answered with the flat-window value, which keeps every output a
       * function of its own window; an epsilon band would carry the quote unit
       * (#253).
       */
      if( den != 0.0 )
         value = -num / den;
      else
         value = flatValue;

      if( limbs == 2 )
      {
         y = inReal[trailingIdx] * scale;
         t = y * invWidth;
         a = (t + 6755399441055744.0) - 6755399441055744.0;
         t = a * width;
         c = y - t;
         denA -= a;
         denC -= c;
         numA -= periodDouble * a;
         numC -= periodDouble * c;
      }
      else if( limbs == 3 )
      {
         y = inReal[trailingIdx] * scale;
         t = y * invWidthSq;
         a = (t + 6755399441055744.0) - 6755399441055744.0;
         t = a * widthSq;
         q = y - t;
         t = q * invWidth;
         b = (t + 6755399441055744.0) - 6755399441055744.0;
         t = b * width;
         c = q - t;
         denA -= a;
         denB -= b;
         denC -= c;
         numA -= periodDouble * a;
         numB -= periodDouble * b;
         numC -= periodDouble * c;
      }

      /* After the trailing value is read: outReal may be inReal. */
      outReal[outIdx] = value;
      trailingIdx++;
      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
