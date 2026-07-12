         static int AccbandsLookback( int           optInTimePeriod  /* From 2 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             SubArray<double>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outRealUpperBand,
                                             SubArray<double>^  outRealMiddleBand,
                                             SubArray<double>^  outRealLowerBand );

         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             SubArray<float>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outRealUpperBand,
                                             SubArray<double>^  outRealMiddleBand,
                                             SubArray<double>^  outRealLowerBand );

         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return Accbands( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return Accbands( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand );
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             int optInTimePeriod,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<double>^ inHigh,
                                                  SubArray<double>^ inLow,
                                                  SubArray<double>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outRealUpperBand,
                                                  SubArray<double>^  outRealMiddleBand,
                                                  SubArray<double>^  outRealLowerBand );

         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<float>^ inHigh,
                                                  SubArray<float>^ inLow,
                                                  SubArray<float>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outRealUpperBand,
                                                  SubArray<double>^  outRealMiddleBand,
                                                  SubArray<double>^  outRealLowerBand );

         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outRealUpperBand,
                                                  cli::array<double>^  outRealMiddleBand,
                                                  cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return AccbandsLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outRealUpperBand,
                                                  cli::array<double>^  outRealMiddleBand,
                                                  cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return AccbandsLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outRealUpperBand,
                                                  cli::array<double>^  outRealMiddleBand,
                                                  cli::array<double>^  outRealLowerBand );
         static enum class RetCode AccbandsLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  int optInTimePeriod,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outRealUpperBand,
                                                  cli::array<double>^  outRealMiddleBand,
                                                  cli::array<double>^  outRealLowerBand );
#endif

         #define TA_ACCBANDS Core::Accbands
         #define TA_ACCBANDS_Lookback Core::AccbandsLookback
         #define TA_ACCBANDS_Logic Core::AccbandsLogic
