         static int BbandsLookback( int           optInTimePeriod  /* From 2 to 100000 */, double           optInNbDevUp  /* From -179769313486231570000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 to 179769313486231570000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 */, double           optInNbDevDn  /* From -179769313486231570000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 to 179769313486231570000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 */, int           optInMAType );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outRealUpperBand,
                                           SubArray<double>^  outRealMiddleBand,
                                           SubArray<double>^  outRealLowerBand );

         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outRealUpperBand,
                                           SubArray<double>^  outRealMiddleBand,
                                           SubArray<double>^  outRealLowerBand );

         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return Bbands( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
                         optInNbDevUp,
                         optInNbDevDn,
                         optInMAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return Bbands( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
                         optInNbDevUp,
                         optInNbDevDn,
                         optInMAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand );
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int optInTimePeriod,
                                           double optInNbDevUp,
                                           double optInNbDevDn,
                                           int optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outRealUpperBand,
                                                SubArray<double>^  outRealMiddleBand,
                                                SubArray<double>^  outRealLowerBand );

         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outRealUpperBand,
                                                SubArray<double>^  outRealMiddleBand,
                                                SubArray<double>^  outRealLowerBand );

         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outRealUpperBand,
                                                cli::array<double>^  outRealMiddleBand,
                                                cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return BbandsLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
                         optInNbDevUp,
                         optInNbDevDn,
                         optInMAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outRealUpperBand,
                                                cli::array<double>^  outRealMiddleBand,
                                                cli::array<double>^  outRealLowerBand )
         {
            if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) return RetCode::BadParam;
            return BbandsLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
                         optInNbDevUp,
                         optInNbDevDn,
                         optInMAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
               gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
               gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outRealUpperBand,
                                                cli::array<double>^  outRealMiddleBand,
                                                cli::array<double>^  outRealLowerBand );
         static enum class RetCode BbandsLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                double optInNbDevUp,
                                                double optInNbDevDn,
                                                int optInMAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outRealUpperBand,
                                                cli::array<double>^  outRealMiddleBand,
                                                cli::array<double>^  outRealLowerBand );
#endif

         #define TA_BBANDS Core::Bbands
         #define TA_BBANDS_Lookback Core::BbandsLookback
         #define TA_BBANDS_Logic Core::BbandsLogic
