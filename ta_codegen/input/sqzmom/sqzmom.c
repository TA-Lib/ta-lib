/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#488).
 */

int sqzmom_lookback( int optInBBPeriod,
   double optInNbDev,
   int optInKCPeriod,
   double optInFactorWide,
   double optInFactorNormal,
   double optInFactorNarrow )
{
   int lookbackTotal, leg;

   (void)optInNbDev;
   (void)optInFactorWide;
   (void)optInFactorNormal;
   (void)optInFactorNarrow;

   /* Three chains reach the first bar at which both outputs exist, and the
    * first output is the longest of them:
    *
    *   bands        sma(b) / stddev(b)           = b-1
    *   compression  trange -> sma(k)             = 1 + (k-1) = k
    *   momentum     midprice(k) -> linearreg(k)  = (k-1) + (k-1) = 2(k-1)
    *
    * The Keltner centre, sma(k) on the close, is k-1 and never dominates the
    * momentum chain it also feeds. At b = k = 20 the three read 19, 20 and 38,
    * which is where pandas, ta4j and trading-signals put their first value.
    * The factors scale a band that already exists, so they do not enter.
    */
   lookbackTotal = sma_lookback( optInBBPeriod );

   leg = stddev_lookback( optInBBPeriod, 1.0 );
   if( leg > lookbackTotal )
      lookbackTotal = leg;

   leg = trange_lookback() + sma_lookback( optInKCPeriod );
   if( leg > lookbackTotal )
      lookbackTotal = leg;

   leg = midprice_lookback( optInKCPeriod ) + linearreg_lookback( optInKCPeriod );
   if( leg > lookbackTotal )
      lookbackTotal = leg;

   return lookbackTotal;
}

TA_RetCode sqzmom(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInBBPeriod,
   double optInNbDev,
   int optInKCPeriod,
   double optInFactorWide,
   double optInFactorNormal,
   double optInFactorNarrow,
   int *outBegIdx, int *outNBElement,
   double outMomentum[],
   int outSqueeze[])
{
   TA_RetCode retCode;
   int lookbackTotal, stageLookback, stageIdx, stageLen, i, n;
   int tempBegIdx, tempNbElement;
   double *tempDev;
   double *tempWork;
   double *tempKCM;
   double *tempMid;
   double *tempSD;
   double up, lo, centre, band;

   /* Every leg is a shipped function, CALLED rather than transcribed, so both
    * outputs carry each callee's own arithmetic bit for bit:
    *
    *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
    *   KCM      = sma(close, k)
    *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
    *   DMID     = midprice(high, low, k)
    *   DEV      = close - (DMID + KCM)/2
    *   momentum = linearreg(DEV, k)
    *
    * The bands are taken as the MA + STDDEV pair rather than through bbands
    * because bbands' SMA fast path states it is bit-identical to that pair, and
    * it is what bbands' own stream tier composes; taking the pair here keeps
    * every optional argument a parameter or a literal.
    *
    * DEV is built from shipped functions too, not in a loop of its own:
    * medprice is (a+b)/2 over any two series, and sma at a period of 1 is the
    * identity -- a one-element sum divided by 1.0, exact for every double --
    * which is what puts the close into the staged range's own indexing so that
    * sub can take the difference.
    *
    * Two chains need their input one window earlier than the first output bar:
    * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
    * one staged range serves both.
    */
   lookbackTotal = sqzmom_lookback( optInBBPeriod, optInNbDev, optInKCPeriod,
      optInFactorWide, optInFactorNormal,
      optInFactorNarrow );

   if( lookbackTotal > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   n = endIdx - startIdx + 1;
   stageLookback = linearreg_lookback( optInKCPeriod );
   stageIdx = startIdx - stageLookback;
   stageLen = n + stageLookback;

   tempDev = malloc( stageLen * sizeof(double) );
   if( !tempDev )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempWork = malloc( stageLen * sizeof(double) );
   if( !tempWork )
   {
      free( tempDev );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempKCM = malloc( n * sizeof(double) );
   if( !tempKCM )
   {
      free( tempDev );
      free( tempWork );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempMid = malloc( n * sizeof(double) );
   if( !tempMid )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempSD = malloc( n * sizeof(double) );
   if( !tempSD )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* The momentum anchor: the Donchian midpoint and the Keltner centre, averaged.
    * midprice is bit-identical to donchian's middle band.
    */
   retCode = midprice( stageIdx, endIdx, inHigh, inLow, optInKCPeriod,
      &tempBegIdx, &tempNbElement, tempDev );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = sma( stageIdx, endIdx, inClose, optInKCPeriod,
      &tempBegIdx, &tempNbElement, tempWork );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* tempDev becomes (DMID + KCM)/2 in place: medprice reads and writes the same
    * index, so the overwrite is safe.
    */
   retCode = medprice( 0, stageLen-1, tempDev, tempWork,
      &tempBegIdx, &tempNbElement, tempDev );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* The close over the staged range, in that range's own indexing. */
   retCode = sma( stageIdx, endIdx, inClose, 1,
      &tempBegIdx, &tempNbElement, tempWork );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = sub( 0, stageLen-1, tempWork, tempDev,
      &tempBegIdx, &tempNbElement, tempDev );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* The compression band: the simple mean of the true range, not an ATR.
    * trange's own lookback is 1, and stageIdx is at least 1 because the
    * compression chain alone puts lookbackTotal at k or more.
    */
   retCode = trange( stageIdx, endIdx, inHigh, inLow, inClose,
      &tempBegIdx, &tempNbElement, tempWork );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = sma( 0, stageLen-1, tempWork, optInKCPeriod,
      &tempBegIdx, &tempNbElement, tempWork );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = sma( startIdx, endIdx, inClose, optInBBPeriod,
      &tempBegIdx, &tempNbElement, tempMid );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = stddev( startIdx, endIdx, inClose, optInBBPeriod, 1.0,
      &tempBegIdx, &tempNbElement, tempSD );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = sma( startIdx, endIdx, inClose, optInKCPeriod,
      &tempBegIdx, &tempNbElement, tempKCM );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* LAST of the calls, because it is the first to write a caller buffer. Every
    * read of high, low and close is above it, so an output aliased onto one of
    * them -- which the abstract layer tests on purpose -- is still intact when it
    * is read.
    *
    * The regression window closes on the first output bar, so entering it at 0 of
    * the staged range puts its first fitted value on startIdx.
    */
   retCode = linearreg( 0, stageLen-1, tempDev, optInKCPeriod,
      &tempBegIdx, &tempNbElement, outMomentum );
   if( retCode != TA_SUCCESS )
   {
      free( tempDev );
      free( tempWork );
      free( tempKCM );
      free( tempMid );
      free( tempSD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* The ordinal. The three Keltner widths share the centre and BAND >= 0, so
    * the tests nest: inside(narrow) implies inside(normal) implies inside(wide).
    * Testing narrow first is what makes the ordinal monotone without comparing
    * the factors to each other, which is why the card asks for
    * `wide >= normal >= narrow >= 0` and not for a strict order.
    */
   for( i = 0; i < n; i++ )
   {
      up = tempMid[i] + tempSD[i]*optInNbDev;
      lo = tempMid[i] - tempSD[i]*optInNbDev;
      centre = tempKCM[i];
      band = tempWork[i];

      if( lo > centre - optInFactorNarrow*band && up < centre + optInFactorNarrow*band )
         outSqueeze[i] = 3;
      else if( lo > centre - optInFactorNormal*band && up < centre + optInFactorNormal*band )
         outSqueeze[i] = 2;
      else if( lo > centre - optInFactorWide*band && up < centre + optInFactorWide*band )
         outSqueeze[i] = 1;
      else if( lo < centre - optInFactorWide*band && up > centre + optInFactorWide*band )
         outSqueeze[i] = -1;
      else
         outSqueeze[i] = 0;
   }

   free( tempDev );
   free( tempWork );
   free( tempKCM );
   free( tempMid );
   free( tempSD );

   *outBegIdx = startIdx;
   *outNBElement = n;

   return TA_SUCCESS;
}
