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
 *  101126 MF,CC  Reject factors out of order (#488).
 *  101126 MF,CC  DEV in one loop instead of three calls (#488).
 *  101126 MF,CC  Both bands from one rounded deviation, as bbands (#488).
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

   /* The range check cannot order the three factors; without this the lookback
    * answers a usable number for a call that cannot run. */
   if( optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal )
      return -1;

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
   double dev, up, lo, centre, band;

   /* Each rolling statistic is a call to the shipped function, so it carries
    * that function's own arithmetic bit for bit:
    *
    *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
    *   KCM      = sma(close, k)
    *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
    *   DMID     = midprice(high, low, k)
    *   DEV      = close - (DMID + KCM)/2
    *   momentum = linearreg(DEV, k)
    *
    * A composed sub-call cannot be handed an MA-type constant, so the bands
    * are the sma + stddev pair and not a bbands call.
    *
    * Two chains need their input one window earlier than the first output bar:
    * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
    * one staged range serves both.
    */

   /* A channel wider than the one it should nest in breaks the ladder: a bar
    * could then be inside "narrow" and outside "normal". Ties are allowed, and
    * wide == normal is the classic on/off/none squeeze.
    */
   if( optInFactorNormal > optInFactorWide || optInFactorNarrow > optInFactorNormal )
      return TA_BAD_PARAM;

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
      outBegIdx, outNBElement, tempDev );
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

   /* DEV, over the staged range. The close is read at the index midprice
    * reported for the range, which is what lets this loop stream: keep
    * outBegIdx as midprice's receiver, and reuse it for no call above here.
    */
   for( i = 0; i < stageLen; i++ )
      tempDev[i] = inClose[*outBegIdx+i] - (tempDev[i]+tempWork[i])/2.0;

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

   /* The ordinal. Every comparison is strict: a band exactly on a channel
    * edge is not inside that channel and not outside it either.
    *
    * Keep the deviation a rounded product added to and subtracted from the
    * middle band, as bbands does. Written as one expression per band, the
    * upper one contracts into a fused multiply-add and the lower one cannot,
    * and a band within one ulp of an edge then changes level.
    */
   for( i = 0; i < n; i++ )
   {
      dev = tempSD[i]*optInNbDev;
      up = tempMid[i] + dev;
      lo = tempMid[i] - dev;
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
