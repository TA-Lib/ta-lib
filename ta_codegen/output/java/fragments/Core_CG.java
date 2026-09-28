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

   /**
    * Number of leading input bars {@link Core#cg} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int cgLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode cgImpl( int startIdx,
                   int endIdx,
                   double inReal[],
                   int optInTimePeriod,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int j = 0;
      int k = 0;
      int limbs = 0;
      int fits = 0;
      int maxAt = 0;
      int failAt = 0;
      int stickyBars = 0;
      int fit2Mid = 0;
      int fit2Lo = 0;
      int fit3Mid = 0;
      double num = 0;
      double den = 0;
      double value = 0;
      double periodDouble = 0;
      double flatValue = 0;
      double weightTotal = 0;
      double width2 = 0;
      double width3 = 0;
      double ylim2 = 0;
      double ylim3 = 0;
      double head2 = 0;
      double head3 = 0;
      double scale = 0;
      double width = 0;
      double invWidth = 0;
      double widthSq = 0;
      double invWidthSq = 0;
      double ylim = 0;
      double maxAbs = 0;
      double half = 0;
      double scale2Mid = 0;
      double scale2Lo = 0;
      double scale3Mid = 0;
      double scale3Lo = 0;
      double x = 0;
      double y = 0;
      double a = 0;
      double b = 0;
      double c = 0;
      double q = 0;
      double t = 0;
      double denA = 0;
      double denB = 0;
      double denC = 0;
      double numA = 0;
      double numB = 0;
      double numC = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
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
      while( weightTotal < periodDouble * (periodDouble + 1.0) * 0.5 ) {
         weightTotal *= 2.0;
      }
      width2 = 9.007199254740992e15 / weightTotal;
      ylim2 = 0.25 * width2 * width2;
      width3 = width2;
      if( width3 > 67108864.0 ) {
         width3 = 67108864.0;
      }
      ylim3 = 0.25 * width2 * width3 * width3;
      /* A scale below the largest that fits leaves room for the window's
       * magnitude to grow before a rebuild, split evenly with the room left for
       * finer values.
       */
      head2 = 1.0;
      while( head2 * head2 * 9.007199254740992e15 < ylim2 ) {
         head2 *= 2.0;
      }
      head3 = 1.0;
      while( head3 * head3 * 9.007199254740992e15 < ylim3 ) {
         head3 *= 2.0;
      }
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
      while( today <= endIdx ) {
         /* Between bars the sums hold the window less its oldest value. */
         fits = 0;
         x = inReal[today];
         y = x * scale;
         if( limbs == 2 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidth;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * width;
               c = y - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
                  denA += a;
                  denC += c;
                  numA += denA;
                  numC += denC;
                  fits = 1;
               }
            }
         } else if( limbs == 3 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidthSq;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * widthSq;
               q = y - t;
               t = q * invWidth;
               b = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = b * width;
               c = q - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
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
         if( fits == 0 && limbs == 0 && stickyBars > 0 ) {
            /* Both witnesses of the last failed fit are still in the window, so
             * it cannot fit either: its largest value can only be larger, which
             * only coarsens the scale the failing value already missed.
             */
            stickyBars -= 1;
            num = 0.0;
            den = 0.0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               den += inReal[j];
               num += den;
            }
         } else if( fits == 0 ) {
            /* The incoming value does not fit the scale: pick one from the
             * window alone.
             */
            maxAbs = 0.0;
            maxAt = 0;
            k = 0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = Math.abs(inReal[j]);
               if( x >= maxAbs ) {
                  maxAbs = x;
                  maxAt = k;
               }
               k += 1;
            }
            /* The largest power of two not above the window's largest
             * magnitude. The search may start from any finite power of two, so
             * it starts from the last window's; keep it uncapped, or a start far
             * from the answer stops short and the fit then depends on the
             * previous window. A window with an infinite magnitude fails the fit
             * at any scale, so it keeps the last one.
             */
            if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 ) {
               while( half * 65536.0 <= maxAbs ) {
                  half *= 65536.0;
               }
               while( half * 2.0 <= maxAbs ) {
                  half *= 2.0;
               }
               while( half > maxAbs * 65536.0 ) {
                  half *= 0.0000152587890625;
               }
               while( half > maxAbs ) {
                  half *= 0.5;
               }
            }
            /* The largest scale each limb count allows, and the same with
             * headroom, at most 2^1022 so that the scale itself is finite. The
             * largest 3-limb scale is the most permissive there is: the window
             * fits some scale only if every value is an integer at that one. A
             * scale below 1 can round a small value to 0, which is not a fit.
             */
            scale2Lo = ylim2 * 0.5 / half;
            if( scale2Lo > 4.49423283715579e307 ) {
               scale2Lo = 4.49423283715579e307;
            }
            scale2Mid = scale2Lo / head2;
            scale3Lo = ylim3 * 0.5 / half;
            if( scale3Lo > 4.49423283715579e307 ) {
               scale3Lo = 4.49423283715579e307;
            }
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
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = inReal[j];
               den += x;
               num += den;
               y = Math.abs(x * scale2Mid);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Mid = 0;
               }
               y = Math.abs(x * scale2Lo);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Lo = 0;
               }
               y = Math.abs(x * scale3Mid);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit3Mid = 0;
               }
               y = Math.abs(x * scale3Lo);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  failAt = k;
               }
               k += 1;
            }
            limbs = 0;
            if( failAt >= 0 ) {
               if( failAt < maxAt ) {
                  stickyBars = failAt;
               } else {
                  stickyBars = maxAt;
               }
            } else {
               if( fit2Mid == 1 || fit2Lo == 1 ) {
                  limbs = 2;
                  width = width2;
                  ylim = ylim2;
                  if( fit2Mid == 1 ) {
                     scale = scale2Mid;
                  } else {
                     scale = scale2Lo;
                  }
               } else {
                  limbs = 3;
                  width = width3;
                  ylim = ylim3;
                  if( fit3Mid == 1 ) {
                     scale = scale3Mid;
                  } else {
                     scale = scale3Lo;
                  }
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
               if( limbs == 2 ) {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = inReal[j] * scale;
                     t = y * invWidth;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * width;
                     c = y - t;
                     denA += a;
                     denC += c;
                     numA += denA;
                     numC += denC;
                  }
               } else {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = inReal[j] * scale;
                     t = y * invWidthSq;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * widthSq;
                     q = y - t;
                     t = q * invWidth;
                     b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( limbs == 2 ) {
            t = denA * width;
            den = t + denC;
            t = numA * width;
            num = t + numC;
         } else if( limbs == 3 ) {
            t = denC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = denB + q;
            t = q * width;
            c = denC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = q * width;
            b = b - t;
            t = b * width;
            c = t + c;
            t = denA + q;
            t = t * widthSq;
            den = t + c;
            t = numC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = numB + q;
            t = q * width;
            c = numC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( den != 0.0 ) {
            value = -num / den;
         } else {
            value = flatValue;
         }
         if( limbs == 2 ) {
            y = inReal[trailingIdx] * scale;
            t = y * invWidth;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * width;
            c = y - t;
            denA -= a;
            denC -= c;
            numA -= periodDouble * a;
            numC -= periodDouble * c;
         } else if( limbs == 3 ) {
            y = inReal[trailingIdx] * scale;
            t = y * invWidthSq;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         trailingIdx += 1;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode cgImpl( int startIdx,
                   int endIdx,
                   float inReal[],
                   int optInTimePeriod,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int j = 0;
      int k = 0;
      int limbs = 0;
      int fits = 0;
      int maxAt = 0;
      int failAt = 0;
      int stickyBars = 0;
      int fit2Mid = 0;
      int fit2Lo = 0;
      int fit3Mid = 0;
      double num = 0;
      double den = 0;
      double value = 0;
      double periodDouble = 0;
      double flatValue = 0;
      double weightTotal = 0;
      double width2 = 0;
      double width3 = 0;
      double ylim2 = 0;
      double ylim3 = 0;
      double head2 = 0;
      double head3 = 0;
      double scale = 0;
      double width = 0;
      double invWidth = 0;
      double widthSq = 0;
      double invWidthSq = 0;
      double ylim = 0;
      double maxAbs = 0;
      double half = 0;
      double scale2Mid = 0;
      double scale2Lo = 0;
      double scale3Mid = 0;
      double scale3Lo = 0;
      double x = 0;
      double y = 0;
      double a = 0;
      double b = 0;
      double c = 0;
      double q = 0;
      double t = 0;
      double denA = 0;
      double denB = 0;
      double denC = 0;
      double numA = 0;
      double numB = 0;
      double numC = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      periodDouble = (double)optInTimePeriod;
      flatValue = -(periodDouble + 1.0) * 0.5;
      weightTotal = 1.0;
      while( weightTotal < periodDouble * (periodDouble + 1.0) * 0.5 ) {
         weightTotal *= 2.0;
      }
      width2 = 9.007199254740992e15 / weightTotal;
      ylim2 = 0.25 * width2 * width2;
      width3 = width2;
      if( width3 > 67108864.0 ) {
         width3 = 67108864.0;
      }
      ylim3 = 0.25 * width2 * width3 * width3;
      head2 = 1.0;
      while( head2 * head2 * 9.007199254740992e15 < ylim2 ) {
         head2 *= 2.0;
      }
      head3 = 1.0;
      while( head3 * head3 * 9.007199254740992e15 < ylim3 ) {
         head3 *= 2.0;
      }
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
      while( today <= endIdx ) {
         fits = 0;
         x = (double)inReal[today];
         y = x * scale;
         if( limbs == 2 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidth;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * width;
               c = y - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
                  denA += a;
                  denC += c;
                  numA += denA;
                  numC += denC;
                  fits = 1;
               }
            }
         } else if( limbs == 3 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidthSq;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * widthSq;
               q = y - t;
               t = q * invWidth;
               b = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = b * width;
               c = q - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
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
         if( fits == 0 && limbs == 0 && stickyBars > 0 ) {
            stickyBars -= 1;
            num = 0.0;
            den = 0.0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               den += (double)inReal[j];
               num += den;
            }
         } else if( fits == 0 ) {
            maxAbs = 0.0;
            maxAt = 0;
            k = 0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = Math.abs((double)inReal[j]);
               if( x >= maxAbs ) {
                  maxAbs = x;
                  maxAt = k;
               }
               k += 1;
            }
            if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 ) {
               while( half * 65536.0 <= maxAbs ) {
                  half *= 65536.0;
               }
               while( half * 2.0 <= maxAbs ) {
                  half *= 2.0;
               }
               while( half > maxAbs * 65536.0 ) {
                  half *= 0.0000152587890625;
               }
               while( half > maxAbs ) {
                  half *= 0.5;
               }
            }
            scale2Lo = ylim2 * 0.5 / half;
            if( scale2Lo > 4.49423283715579e307 ) {
               scale2Lo = 4.49423283715579e307;
            }
            scale2Mid = scale2Lo / head2;
            scale3Lo = ylim3 * 0.5 / half;
            if( scale3Lo > 4.49423283715579e307 ) {
               scale3Lo = 4.49423283715579e307;
            }
            scale3Mid = scale3Lo / head3;
            fit2Mid = 1;
            fit2Lo = 1;
            fit3Mid = 1;
            failAt = -1;
            num = 0.0;
            den = 0.0;
            k = 0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = (double)inReal[j];
               den += x;
               num += den;
               y = Math.abs(x * scale2Mid);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Mid = 0;
               }
               y = Math.abs(x * scale2Lo);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Lo = 0;
               }
               y = Math.abs(x * scale3Mid);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit3Mid = 0;
               }
               y = Math.abs(x * scale3Lo);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  failAt = k;
               }
               k += 1;
            }
            limbs = 0;
            if( failAt >= 0 ) {
               if( failAt < maxAt ) {
                  stickyBars = failAt;
               } else {
                  stickyBars = maxAt;
               }
            } else {
               if( fit2Mid == 1 || fit2Lo == 1 ) {
                  limbs = 2;
                  width = width2;
                  ylim = ylim2;
                  if( fit2Mid == 1 ) {
                     scale = scale2Mid;
                  } else {
                     scale = scale2Lo;
                  }
               } else {
                  limbs = 3;
                  width = width3;
                  ylim = ylim3;
                  if( fit3Mid == 1 ) {
                     scale = scale3Mid;
                  } else {
                     scale = scale3Lo;
                  }
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
               if( limbs == 2 ) {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = (double)inReal[j] * scale;
                     t = y * invWidth;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * width;
                     c = y - t;
                     denA += a;
                     denC += c;
                     numA += denA;
                     numC += denC;
                  }
               } else {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = (double)inReal[j] * scale;
                     t = y * invWidthSq;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * widthSq;
                     q = y - t;
                     t = q * invWidth;
                     b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( limbs == 2 ) {
            t = denA * width;
            den = t + denC;
            t = numA * width;
            num = t + numC;
         } else if( limbs == 3 ) {
            t = denC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = denB + q;
            t = q * width;
            c = denC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = q * width;
            b = b - t;
            t = b * width;
            c = t + c;
            t = denA + q;
            t = t * widthSq;
            den = t + c;
            t = numC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = numB + q;
            t = q * width;
            c = numC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = q * width;
            b = b - t;
            t = b * width;
            c = t + c;
            t = numA + q;
            t = t * widthSq;
            num = t + c;
         }
         if( den != 0.0 ) {
            value = -num / den;
         } else {
            value = flatValue;
         }
         if( limbs == 2 ) {
            y = (double)inReal[trailingIdx] * scale;
            t = y * invWidth;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * width;
            c = y - t;
            denA -= a;
            denC -= c;
            numA -= periodDouble * a;
            numC -= periodDouble * c;
         } else if( limbs == 3 ) {
            y = (double)inReal[trailingIdx] * scale;
            t = y * invWidthSq;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = b * width;
            c = q - t;
            denA -= a;
            denB -= b;
            denC -= c;
            numA -= periodDouble * a;
            numB -= periodDouble * b;
            numC -= periodDouble * c;
         }
         outReal[outIdx] = value;
         trailingIdx += 1;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * John Ehlers' Center of Gravity oscillator: the balance point of the last
    * {@code optInTimePeriod} values, each weighted by its value and placed at
    * its position counting back from the current bar, which is position 1,
    * negated so that it rises with price. It is smooth and has essentially no
    * lag. Ehlers reads turning points from it and trades its crossings with a
    * copy of itself delayed by one bar. For a positive series it stays between
    * -optInTimePeriod and -1, and a flat window sits at the midpoint,
    * -(optInTimePeriod+1)/2.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cg">ta-lib.org/functions/cg</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.</li>
    * <li>His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.</li>
    * <li>Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.</li>
    * <li>Both sums are exact before the divide, so a window summing to exactly zero is always recognised and the value does not depend on where the call started. The exception is a window holding a non-finite value, a value with bits below 2^-1022 (every subnormal, and only values under about 2e-292), or values that together span more binary digits than the sums can hold exactly (97 at the default period, 84 at 1000, 58 at 100000): it is summed in floating point, oldest value first.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#cgLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Negated center of gravity of the window. Must hold at least
    *        {@code endIdx - max(startIdx, cgLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#wma
    * @see Core#medprice
    * @see Core#cti
    */
   public OutRange cg( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("CG", startIdx, endIdx);
      int guardStart = clampedStart("CG", startIdx, cgLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CG", "inReal", inReal, guardInLen);
      requireLength("CG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cgImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * John Ehlers' Center of Gravity oscillator: the balance point of the last
    * {@code optInTimePeriod} values, each weighted by its value and placed at
    * its position counting back from the current bar, which is position 1,
    * negated so that it rises with price. It is smooth and has essentially no
    * lag. Ehlers reads turning points from it and trades its crossings with a
    * copy of itself delayed by one bar. For a positive series it stays between
    * -optInTimePeriod and -1, and a flat window sits at the midpoint,
    * -(optInTimePeriod+1)/2.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cg">ta-lib.org/functions/cg</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.</li>
    * <li>His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.</li>
    * <li>Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.</li>
    * <li>Both sums are exact before the divide, so a window summing to exactly zero is always recognised and the value does not depend on where the call started. The exception is a window holding a non-finite value, a value with bits below 2^-1022 (every subnormal, and only values under about 2e-292), or values that together span more binary digits than the sums can hold exactly (97 at the default period, 84 at 1000, 58 at 100000): it is summed in floating point, oldest value first.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#cgLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Negated center of gravity of the window. Must hold at least
    *        {@code endIdx - max(startIdx, cgLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#wma
    * @see Core#medprice
    * @see Core#cti
    */
   public OutRange cg( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("CG", startIdx, endIdx);
      int guardStart = clampedStart("CG", startIdx, cgLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CG", "inReal", inReal, guardInLen);
      requireLength("CG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cgImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CG stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cg} over the same series.
    * Open with {@link Core#cgOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CgStream {
      private Core core;
      private int optInTimePeriod;
      private int lookbackTotal;
      private int limbs;
      private int stickyBars;
      private double num;
      private double den;
      private double periodDouble;
      private double flatValue;
      private double width2;
      private double width3;
      private double ylim2;
      private double ylim3;
      private double head2;
      private double head3;
      private double scale;
      private double width;
      private double invWidth;
      private double widthSq;
      private double invWidthSq;
      private double ylim;
      private double half;
      private double denA;
      private double denB;
      private double denC;
      private double numA;
      private double numB;
      private double numC;
      private int ringPos_trailingIdx;
      private int ringCap_trailingIdx;
      private double[] ring_trailingIdx_inReal;
      private int winPos_j;
      private int winCap_j;
      private double[] win_j_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CgStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#cg} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CG advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CgStream( CgStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lookbackTotal = other.lookbackTotal;
         this.limbs = other.limbs;
         this.stickyBars = other.stickyBars;
         this.num = other.num;
         this.den = other.den;
         this.periodDouble = other.periodDouble;
         this.flatValue = other.flatValue;
         this.width2 = other.width2;
         this.width3 = other.width3;
         this.ylim2 = other.ylim2;
         this.ylim3 = other.ylim3;
         this.head2 = other.head2;
         this.head3 = other.head3;
         this.scale = other.scale;
         this.width = other.width;
         this.invWidth = other.invWidth;
         this.widthSq = other.widthSq;
         this.invWidthSq = other.invWidthSq;
         this.ylim = other.ylim;
         this.half = other.half;
         this.denA = other.denA;
         this.denB = other.denB;
         this.denC = other.denC;
         this.numA = other.numA;
         this.numB = other.numB;
         this.numC = other.numC;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.winPos_j = other.winPos_j;
         this.winCap_j = other.winCap_j;
         this.win_j_inReal = other.win_j_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CG update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("CG update", "inReal");
         core.cgStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("CG peek", "inReal");
         CgStream sp = this;
         int j = 0;
         int k = 0;
         int fits = 0;
         int maxAt = 0;
         int failAt = 0;
         int fit2Mid = 0;
         int fit2Lo = 0;
         int fit3Mid = 0;
         double value = 0.0;
         double maxAbs = 0.0;
         double scale2Mid = 0.0;
         double scale2Lo = 0.0;
         double scale3Mid = 0.0;
         double scale3Lo = 0.0;
         double x = 0.0;
         double y = 0.0;
         double a = 0.0;
         double b = 0.0;
         double c = 0.0;
         double q = 0.0;
         double t = 0.0;
         double cur_outReal = 0.0;
         double den = sp.den;
         double denA = sp.denA;
         double denB = sp.denB;
         double denC = sp.denC;
         double half = sp.half;
         double invWidth = sp.invWidth;
         double invWidthSq = sp.invWidthSq;
         int limbs = sp.limbs;
         double num = sp.num;
         double numA = sp.numA;
         double numB = sp.numB;
         double numC = sp.numC;
         double scale = sp.scale;
         int stickyBars = sp.stickyBars;
         double width = sp.width;
         double widthSq = sp.widthSq;
         double ylim = sp.ylim;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         pkSlot1 = sp.winPos_j;
         pkVal1 = inReal;
         /* Between bars the sums hold the window less its oldest value. */
         fits = 0;
         x = inReal;
         y = x * scale;
         if( limbs == 2 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidth;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * width;
               c = y - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
                  denA += a;
                  denC += c;
                  numA += denA;
                  numC += denC;
                  fits = 1;
               }
            }
         } else if( limbs == 3 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidthSq;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * widthSq;
               q = y - t;
               t = q * invWidth;
               b = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = b * width;
               c = q - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
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
         if( fits == 0 && limbs == 0 && stickyBars > 0 ) {
            /* Both witnesses of the last failed fit are still in the window, so
             * it cannot fit either: its largest value can only be larger, which
             * only coarsens the scale the failing value already missed.
             */
            stickyBars -= 1;
            num = 0.0;
            den = 0.0;
            for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
               den += (((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal1;
               num += den;
            }
         } else if( fits == 0 ) {
            /* The incoming value does not fit the scale: pick one from the
             * window alone.
             */
            maxAbs = 0.0;
            maxAt = 0;
            k = 0;
            for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
               x = Math.abs((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal1);
               if( x >= maxAbs ) {
                  maxAbs = x;
                  maxAt = k;
               }
               k += 1;
            }
            /* The largest power of two not above the window's largest
             * magnitude. The search may start from any finite power of two, so
             * it starts from the last window's; keep it uncapped, or a start far
             * from the answer stops short and the fit then depends on the
             * previous window. A window with an infinite magnitude fails the fit
             * at any scale, so it keeps the last one.
             */
            if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 ) {
               while( half * 65536.0 <= maxAbs ) {
                  half *= 65536.0;
               }
               while( half * 2.0 <= maxAbs ) {
                  half *= 2.0;
               }
               while( half > maxAbs * 65536.0 ) {
                  half *= 0.0000152587890625;
               }
               while( half > maxAbs ) {
                  half *= 0.5;
               }
            }
            /* The largest scale each limb count allows, and the same with
             * headroom, at most 2^1022 so that the scale itself is finite. The
             * largest 3-limb scale is the most permissive there is: the window
             * fits some scale only if every value is an integer at that one. A
             * scale below 1 can round a small value to 0, which is not a fit.
             */
            scale2Lo = sp.ylim2 * 0.5 / half;
            if( scale2Lo > 4.49423283715579e307 ) {
               scale2Lo = 4.49423283715579e307;
            }
            scale2Mid = scale2Lo / sp.head2;
            scale3Lo = sp.ylim3 * 0.5 / half;
            if( scale3Lo > 4.49423283715579e307 ) {
               scale3Lo = 4.49423283715579e307;
            }
            scale3Mid = scale3Lo / sp.head3;
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
            for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
               x = (((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal1;
               den += x;
               num += den;
               y = Math.abs(x * scale2Mid);
               if( !(y < sp.ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Mid = 0;
               }
               y = Math.abs(x * scale2Lo);
               if( !(y < sp.ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Lo = 0;
               }
               y = Math.abs(x * scale3Mid);
               if( !(y < sp.ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit3Mid = 0;
               }
               y = Math.abs(x * scale3Lo);
               if( !(y < sp.ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  failAt = k;
               }
               k += 1;
            }
            limbs = 0;
            if( failAt >= 0 ) {
               if( failAt < maxAt ) {
                  stickyBars = failAt;
               } else {
                  stickyBars = maxAt;
               }
            } else {
               if( fit2Mid == 1 || fit2Lo == 1 ) {
                  limbs = 2;
                  width = sp.width2;
                  ylim = sp.ylim2;
                  if( fit2Mid == 1 ) {
                     scale = scale2Mid;
                  } else {
                     scale = scale2Lo;
                  }
               } else {
                  limbs = 3;
                  width = sp.width3;
                  ylim = sp.ylim3;
                  if( fit3Mid == 1 ) {
                     scale = scale3Mid;
                  } else {
                     scale = scale3Lo;
                  }
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
               if( limbs == 2 ) {
                  for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
                     y = ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal1) * scale;
                     t = y * invWidth;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * width;
                     c = y - t;
                     denA += a;
                     denC += c;
                     numA += denA;
                     numC += denC;
                  }
               } else {
                  for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
                     y = ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal1) * scale;
                     t = y * invWidthSq;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * widthSq;
                     q = y - t;
                     t = q * invWidth;
                     b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( limbs == 2 ) {
            t = denA * width;
            den = t + denC;
            t = numA * width;
            num = t + numC;
         } else if( limbs == 3 ) {
            t = denC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = denB + q;
            t = q * width;
            c = denC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = q * width;
            b = b - t;
            t = b * width;
            c = t + c;
            t = denA + q;
            t = t * widthSq;
            den = t + c;
            t = numC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = numB + q;
            t = q * width;
            c = numC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( den != 0.0 ) {
            value = -num / den;
         } else {
            value = sp.flatValue;
         }
         if( limbs == 2 ) {
            y = ((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0) * scale;
            t = y * invWidth;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * width;
            c = y - t;
            denA -= a;
            denC -= c;
            numA -= sp.periodDouble * a;
            numC -= sp.periodDouble * c;
         } else if( limbs == 3 ) {
            y = ((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0) * scale;
            t = y * invWidthSq;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = b * width;
            c = q - t;
            denA -= a;
            denB -= b;
            denC -= c;
            numA -= sp.periodDouble * a;
            numB -= sp.periodDouble * b;
            numC -= sp.periodDouble * c;
         }
         /* After the trailing value is read: outReal may be inReal. */
         cur_outReal = value;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public CgStream clone() {
         return new CgStream(this);
      }
   }
   private void cgStepImpl( CgStream sp, double inReal )
   {
      int j = 0;
      int k = 0;
      int fits = 0;
      int maxAt = 0;
      int failAt = 0;
      int fit2Mid = 0;
      int fit2Lo = 0;
      int fit3Mid = 0;
      double value = 0.0;
      double maxAbs = 0.0;
      double scale2Mid = 0.0;
      double scale2Lo = 0.0;
      double scale3Mid = 0.0;
      double scale3Lo = 0.0;
      double x = 0.0;
      double y = 0.0;
      double a = 0.0;
      double b = 0.0;
      double c = 0.0;
      double q = 0.0;
      double t = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.win_j_inReal[sp.winPos_j] = inReal;
      /* Between bars the sums hold the window less its oldest value. */
      fits = 0;
      x = inReal;
      y = x * sp.scale;
      if( sp.limbs == 2 ) {
         if( Math.abs(y) < sp.ylim && (y != 0.0 || x == 0.0) ) {
            t = y * sp.invWidth;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * sp.width;
            c = y - t;
            if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
               sp.denA += a;
               sp.denC += c;
               sp.numA += sp.denA;
               sp.numC += sp.denC;
               fits = 1;
            }
         }
      } else if( sp.limbs == 3 ) {
         if( Math.abs(y) < sp.ylim && (y != 0.0 || x == 0.0) ) {
            t = y * sp.invWidthSq;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * sp.widthSq;
            q = y - t;
            t = q * sp.invWidth;
            b = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = b * sp.width;
            c = q - t;
            if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
               sp.denA += a;
               sp.denB += b;
               sp.denC += c;
               sp.numA += sp.denA;
               sp.numB += sp.denB;
               sp.numC += sp.denC;
               fits = 1;
            }
         }
      }
      if( fits == 0 && sp.limbs == 0 && sp.stickyBars > 0 ) {
         /* Both witnesses of the last failed fit are still in the window, so
          * it cannot fit either: its largest value can only be larger, which
          * only coarsens the scale the failing value already missed.
          */
         sp.stickyBars -= 1;
         sp.num = 0.0;
         sp.den = 0.0;
         for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
            sp.den += sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
            sp.num += sp.den;
         }
      } else if( fits == 0 ) {
         /* The incoming value does not fit the scale: pick one from the
          * window alone.
          */
         maxAbs = 0.0;
         maxAt = 0;
         k = 0;
         for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
            x = Math.abs(sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j]);
            if( x >= maxAbs ) {
               maxAbs = x;
               maxAt = k;
            }
            k += 1;
         }
         /* The largest power of two not above the window's largest
          * magnitude. The search may start from any finite power of two, so
          * it starts from the last window's; keep it uncapped, or a start far
          * from the answer stops short and the fit then depends on the
          * previous window. A window with an infinite magnitude fails the fit
          * at any scale, so it keeps the last one.
          */
         if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 ) {
            while( sp.half * 65536.0 <= maxAbs ) {
               sp.half *= 65536.0;
            }
            while( sp.half * 2.0 <= maxAbs ) {
               sp.half *= 2.0;
            }
            while( sp.half > maxAbs * 65536.0 ) {
               sp.half *= 0.0000152587890625;
            }
            while( sp.half > maxAbs ) {
               sp.half *= 0.5;
            }
         }
         /* The largest scale each limb count allows, and the same with
          * headroom, at most 2^1022 so that the scale itself is finite. The
          * largest 3-limb scale is the most permissive there is: the window
          * fits some scale only if every value is an integer at that one. A
          * scale below 1 can round a small value to 0, which is not a fit.
          */
         scale2Lo = sp.ylim2 * 0.5 / sp.half;
         if( scale2Lo > 4.49423283715579e307 ) {
            scale2Lo = 4.49423283715579e307;
         }
         scale2Mid = scale2Lo / sp.head2;
         scale3Lo = sp.ylim3 * 0.5 / sp.half;
         if( scale3Lo > 4.49423283715579e307 ) {
            scale3Lo = 4.49423283715579e307;
         }
         scale3Mid = scale3Lo / sp.head3;
         /* The same pass sums the window as it stands, oldest first, for when
          * nothing fits: non-finite values, bits below 2^-1022, or too wide a
          * span of magnitudes.
          */
         fit2Mid = 1;
         fit2Lo = 1;
         fit3Mid = 1;
         failAt = -1;
         sp.num = 0.0;
         sp.den = 0.0;
         k = 0;
         for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
            x = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
            sp.den += x;
            sp.num += sp.den;
            y = Math.abs(x * scale2Mid);
            if( !(y < sp.ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
               fit2Mid = 0;
            }
            y = Math.abs(x * scale2Lo);
            if( !(y < sp.ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
               fit2Lo = 0;
            }
            y = Math.abs(x * scale3Mid);
            if( !(y < sp.ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
               fit3Mid = 0;
            }
            y = Math.abs(x * scale3Lo);
            if( !(y < sp.ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
               failAt = k;
            }
            k += 1;
         }
         sp.limbs = 0;
         if( failAt >= 0 ) {
            if( failAt < maxAt ) {
               sp.stickyBars = failAt;
            } else {
               sp.stickyBars = maxAt;
            }
         } else {
            if( fit2Mid == 1 || fit2Lo == 1 ) {
               sp.limbs = 2;
               sp.width = sp.width2;
               sp.ylim = sp.ylim2;
               if( fit2Mid == 1 ) {
                  sp.scale = scale2Mid;
               } else {
                  sp.scale = scale2Lo;
               }
            } else {
               sp.limbs = 3;
               sp.width = sp.width3;
               sp.ylim = sp.ylim3;
               if( fit3Mid == 1 ) {
                  sp.scale = scale3Mid;
               } else {
                  sp.scale = scale3Lo;
               }
            }
            sp.invWidth = 1.0 / sp.width;
            sp.widthSq = sp.width * sp.width;
            sp.invWidthSq = sp.invWidth * sp.invWidth;
            sp.denA = 0.0;
            sp.denB = 0.0;
            sp.denC = 0.0;
            sp.numA = 0.0;
            sp.numB = 0.0;
            sp.numC = 0.0;
            if( sp.limbs == 2 ) {
               for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
                  y = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] * sp.scale;
                  t = y * sp.invWidth;
                  a = t + 6.755399441055744e15 - 6.755399441055744e15;
                  t = a * sp.width;
                  c = y - t;
                  sp.denA += a;
                  sp.denC += c;
                  sp.numA += sp.denA;
                  sp.numC += sp.denC;
               }
            } else {
               for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
                  y = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] * sp.scale;
                  t = y * sp.invWidthSq;
                  a = t + 6.755399441055744e15 - 6.755399441055744e15;
                  t = a * sp.widthSq;
                  q = y - t;
                  t = q * sp.invWidth;
                  b = t + 6.755399441055744e15 - 6.755399441055744e15;
                  t = b * sp.width;
                  c = q - t;
                  sp.denA += a;
                  sp.denB += b;
                  sp.denC += c;
                  sp.numA += sp.denA;
                  sp.numB += sp.denB;
                  sp.numC += sp.denC;
               }
            }
         }
      }
      /* One rounding each: the limbs are carried into range first, so the
       * last add sees two exact values.
       */
      if( sp.limbs == 2 ) {
         t = sp.denA * sp.width;
         sp.den = t + sp.denC;
         t = sp.numA * sp.width;
         sp.num = t + sp.numC;
      } else if( sp.limbs == 3 ) {
         t = sp.denC * sp.invWidth;
         q = t + 6.755399441055744e15 - 6.755399441055744e15;
         b = sp.denB + q;
         t = q * sp.width;
         c = sp.denC - t;
         t = b * sp.invWidth;
         q = t + 6.755399441055744e15 - 6.755399441055744e15;
         t = q * sp.width;
         b = b - t;
         t = b * sp.width;
         c = t + c;
         t = sp.denA + q;
         t = t * sp.widthSq;
         sp.den = t + c;
         t = sp.numC * sp.invWidth;
         q = t + 6.755399441055744e15 - 6.755399441055744e15;
         b = sp.numB + q;
         t = q * sp.width;
         c = sp.numC - t;
         t = b * sp.invWidth;
         q = t + 6.755399441055744e15 - 6.755399441055744e15;
         t = q * sp.width;
         b = b - t;
         t = b * sp.width;
         c = t + c;
         t = sp.numA + q;
         t = t * sp.widthSq;
         sp.num = t + c;
      }
      /* The denominator is a signed sum, so only an exact zero is degenerate.
       * It is answered with the flat-window value, which keeps every output a
       * function of its own window; an epsilon band would carry the quote unit
       * (#253).
       */
      if( sp.den != 0.0 ) {
         value = -sp.num / sp.den;
      } else {
         value = sp.flatValue;
      }
      if( sp.limbs == 2 ) {
         y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.scale;
         t = y * sp.invWidth;
         a = t + 6.755399441055744e15 - 6.755399441055744e15;
         t = a * sp.width;
         c = y - t;
         sp.denA -= a;
         sp.denC -= c;
         sp.numA -= sp.periodDouble * a;
         sp.numC -= sp.periodDouble * c;
      } else if( sp.limbs == 3 ) {
         y = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.scale;
         t = y * sp.invWidthSq;
         a = t + 6.755399441055744e15 - 6.755399441055744e15;
         t = a * sp.widthSq;
         q = y - t;
         t = q * sp.invWidth;
         b = t + 6.755399441055744e15 - 6.755399441055744e15;
         t = b * sp.width;
         c = q - t;
         sp.denA -= a;
         sp.denB -= b;
         sp.denC -= c;
         sp.numA -= sp.periodDouble * a;
         sp.numB -= sp.periodDouble * b;
         sp.numC -= sp.periodDouble * c;
      }
      /* After the trailing value is read: outReal may be inReal. */
      sp.cur_outReal = value;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
      sp.winPos_j = sp.winPos_j + 1;
      if( sp.winPos_j >= sp.winCap_j ) {
         sp.winPos_j = 0;
      }
   }
   private RetCode cgOpenImpl( CgStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int j = 0;
      int k = 0;
      int limbs = 0;
      int fits = 0;
      int maxAt = 0;
      int failAt = 0;
      int stickyBars = 0;
      int fit2Mid = 0;
      int fit2Lo = 0;
      int fit3Mid = 0;
      double num = 0;
      double den = 0;
      double value = 0;
      double periodDouble = 0;
      double flatValue = 0;
      double weightTotal = 0;
      double width2 = 0;
      double width3 = 0;
      double ylim2 = 0;
      double ylim3 = 0;
      double head2 = 0;
      double head3 = 0;
      double scale = 0;
      double width = 0;
      double invWidth = 0;
      double widthSq = 0;
      double invWidthSq = 0;
      double ylim = 0;
      double maxAbs = 0;
      double half = 0;
      double scale2Mid = 0;
      double scale2Lo = 0;
      double scale3Mid = 0;
      double scale3Lo = 0;
      double x = 0;
      double y = 0;
      double a = 0;
      double b = 0;
      double c = 0;
      double q = 0;
      double t = 0;
      double denA = 0;
      double denB = 0;
      double denC = 0;
      double numA = 0;
      double numB = 0;
      double numC = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
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
      while( weightTotal < periodDouble * (periodDouble + 1.0) * 0.5 ) {
         weightTotal *= 2.0;
      }
      width2 = 9.007199254740992e15 / weightTotal;
      ylim2 = 0.25 * width2 * width2;
      width3 = width2;
      if( width3 > 67108864.0 ) {
         width3 = 67108864.0;
      }
      ylim3 = 0.25 * width2 * width3 * width3;
      /* A scale below the largest that fits leaves room for the window's
       * magnitude to grow before a rebuild, split evenly with the room left for
       * finer values.
       */
      head2 = 1.0;
      while( head2 * head2 * 9.007199254740992e15 < ylim2 ) {
         head2 *= 2.0;
      }
      head3 = 1.0;
      while( head3 * head3 * 9.007199254740992e15 < ylim3 ) {
         head3 *= 2.0;
      }
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
      while( today <= endIdx ) {
         /* Between bars the sums hold the window less its oldest value. */
         fits = 0;
         x = inReal[today];
         y = x * scale;
         if( limbs == 2 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidth;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * width;
               c = y - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
                  denA += a;
                  denC += c;
                  numA += denA;
                  numC += denC;
                  fits = 1;
               }
            }
         } else if( limbs == 3 ) {
            if( Math.abs(y) < ylim && (y != 0.0 || x == 0.0) ) {
               t = y * invWidthSq;
               a = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = a * widthSq;
               q = y - t;
               t = q * invWidth;
               b = t + 6.755399441055744e15 - 6.755399441055744e15;
               t = b * width;
               c = q - t;
               if( c + 6.755399441055744e15 - 6.755399441055744e15 == c ) {
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
         if( fits == 0 && limbs == 0 && stickyBars > 0 ) {
            /* Both witnesses of the last failed fit are still in the window, so
             * it cannot fit either: its largest value can only be larger, which
             * only coarsens the scale the failing value already missed.
             */
            stickyBars -= 1;
            num = 0.0;
            den = 0.0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               den += inReal[j];
               num += den;
            }
         } else if( fits == 0 ) {
            /* The incoming value does not fit the scale: pick one from the
             * window alone.
             */
            maxAbs = 0.0;
            maxAt = 0;
            k = 0;
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = Math.abs(inReal[j]);
               if( x >= maxAbs ) {
                  maxAbs = x;
                  maxAt = k;
               }
               k += 1;
            }
            /* The largest power of two not above the window's largest
             * magnitude. The search may start from any finite power of two, so
             * it starts from the last window's; keep it uncapped, or a start far
             * from the answer stops short and the fit then depends on the
             * previous window. A window with an infinite magnitude fails the fit
             * at any scale, so it keeps the last one.
             */
            if( maxAbs > 0.0 && maxAbs <= 1.7976931348623157e308 ) {
               while( half * 65536.0 <= maxAbs ) {
                  half *= 65536.0;
               }
               while( half * 2.0 <= maxAbs ) {
                  half *= 2.0;
               }
               while( half > maxAbs * 65536.0 ) {
                  half *= 0.0000152587890625;
               }
               while( half > maxAbs ) {
                  half *= 0.5;
               }
            }
            /* The largest scale each limb count allows, and the same with
             * headroom, at most 2^1022 so that the scale itself is finite. The
             * largest 3-limb scale is the most permissive there is: the window
             * fits some scale only if every value is an integer at that one. A
             * scale below 1 can round a small value to 0, which is not a fit.
             */
            scale2Lo = ylim2 * 0.5 / half;
            if( scale2Lo > 4.49423283715579e307 ) {
               scale2Lo = 4.49423283715579e307;
            }
            scale2Mid = scale2Lo / head2;
            scale3Lo = ylim3 * 0.5 / half;
            if( scale3Lo > 4.49423283715579e307 ) {
               scale3Lo = 4.49423283715579e307;
            }
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
            for( j = today - lookbackTotal; j <= today; j += 1 ) {
               x = inReal[j];
               den += x;
               num += den;
               y = Math.abs(x * scale2Mid);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Mid = 0;
               }
               y = Math.abs(x * scale2Lo);
               if( !(y < ylim2) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit2Lo = 0;
               }
               y = Math.abs(x * scale3Mid);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  fit3Mid = 0;
               }
               y = Math.abs(x * scale3Lo);
               if( !(y < ylim3) || y < 4.503599627370496e15 && y + 4.503599627370496e15 - 4.503599627370496e15 != y || y == 0.0 && x != 0.0 ) {
                  failAt = k;
               }
               k += 1;
            }
            limbs = 0;
            if( failAt >= 0 ) {
               if( failAt < maxAt ) {
                  stickyBars = failAt;
               } else {
                  stickyBars = maxAt;
               }
            } else {
               if( fit2Mid == 1 || fit2Lo == 1 ) {
                  limbs = 2;
                  width = width2;
                  ylim = ylim2;
                  if( fit2Mid == 1 ) {
                     scale = scale2Mid;
                  } else {
                     scale = scale2Lo;
                  }
               } else {
                  limbs = 3;
                  width = width3;
                  ylim = ylim3;
                  if( fit3Mid == 1 ) {
                     scale = scale3Mid;
                  } else {
                     scale = scale3Lo;
                  }
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
               if( limbs == 2 ) {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = inReal[j] * scale;
                     t = y * invWidth;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * width;
                     c = y - t;
                     denA += a;
                     denC += c;
                     numA += denA;
                     numC += denC;
                  }
               } else {
                  for( j = today - lookbackTotal; j <= today; j += 1 ) {
                     y = inReal[j] * scale;
                     t = y * invWidthSq;
                     a = t + 6.755399441055744e15 - 6.755399441055744e15;
                     t = a * widthSq;
                     q = y - t;
                     t = q * invWidth;
                     b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( limbs == 2 ) {
            t = denA * width;
            den = t + denC;
            t = numA * width;
            num = t + numC;
         } else if( limbs == 3 ) {
            t = denC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = denB + q;
            t = q * width;
            c = denC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = q * width;
            b = b - t;
            t = b * width;
            c = t + c;
            t = denA + q;
            t = t * widthSq;
            den = t + c;
            t = numC * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
            b = numB + q;
            t = q * width;
            c = numC - t;
            t = b * invWidth;
            q = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         if( den != 0.0 ) {
            value = -num / den;
         } else {
            value = flatValue;
         }
         if( limbs == 2 ) {
            y = inReal[trailingIdx] * scale;
            t = y * invWidth;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * width;
            c = y - t;
            denA -= a;
            denC -= c;
            numA -= periodDouble * a;
            numC -= periodDouble * c;
         } else if( limbs == 3 ) {
            y = inReal[trailingIdx] * scale;
            t = y * invWidthSq;
            a = t + 6.755399441055744e15 - 6.755399441055744e15;
            t = a * widthSq;
            q = y - t;
            t = q * invWidth;
            b = t + 6.755399441055744e15 - 6.755399441055744e15;
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
         outReal[outIdx * outStride] = value;
         trailingIdx += 1;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      int cap_j = (int)(lookbackTotal + 1);
      if( cap_j < 1 || cap_j > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      double[] capWin_j_inReal = new double[cap_j];
      System.arraycopy(inReal, historyLen - cap_j, capWin_j_inReal, 0, cap_j);
      sp.optInTimePeriod = optInTimePeriod;
      sp.lookbackTotal = lookbackTotal;
      sp.limbs = limbs;
      sp.stickyBars = stickyBars;
      sp.num = num;
      sp.den = den;
      sp.periodDouble = periodDouble;
      sp.flatValue = flatValue;
      sp.width2 = width2;
      sp.width3 = width3;
      sp.ylim2 = ylim2;
      sp.ylim3 = ylim3;
      sp.head2 = head2;
      sp.head3 = head3;
      sp.scale = scale;
      sp.width = width;
      sp.invWidth = invWidth;
      sp.widthSq = widthSq;
      sp.invWidthSq = invWidthSq;
      sp.ylim = ylim;
      sp.half = half;
      sp.denA = denA;
      sp.denB = denB;
      sp.denC = denC;
      sp.numA = numA;
      sp.numB = numB;
      sp.numC = numC;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.winPos_j = 0;
      sp.winCap_j = cap_j;
      sp.win_j_inReal = capWin_j_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* cgOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CgStream cgOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CgStream sp = new CgStream(this);
      RetCode retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CG openAndFill", inReal.length, startIdx, cgLookback(optInTimePeriod));
      }
      throw streamFailure("CG openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind cgOpen (composition seam). */
   CgStream cgOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CgStream sp = new CgStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CG open", inReal.length, startIdx, cgLookback(optInTimePeriod));
      }
      throw streamFailure("CG open", retCode);
   }
   /**
    * Open a live CG stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cg} at that bar.
    * <p>The history must hold at least {@code cgLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CgStream cgOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("CG open", "inReal", inReal);
      requireHistory("CG open", inReal.length);
      return cgOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cgOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cg} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CgStream#outRange()}.
    */
   public CgStream cgOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CG openAndFill", "inReal", inReal);
      requireHistory("CG openAndFill", inReal.length);
      int guardOutLen = openFillCount("CG openAndFill", inReal.length, cgLookback(optInTimePeriod));
      requireLength("CG openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("CG openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cgOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
