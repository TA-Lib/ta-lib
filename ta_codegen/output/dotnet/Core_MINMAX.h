         static int MinmaxLookback( int           optInTimePeriod  /* From 2 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outMin,
                                           SubArray<double>^  outMax );

         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outMin,
                                           SubArray<double>^  outMax );

         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax )
         {
            if( outMin == outMax ) return RetCode::BadParam;
            return Minmax( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMin,0),
               gcnew SubArrayFrom1D<double>(outMax,0) );
         }
         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax )
         {
            if( outMin == outMax ) return RetCode::BadParam;
            return Minmax( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMin,0),
               gcnew SubArrayFrom1D<double>(outMax,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax );
         static enum class RetCode Minmax( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int optInTimePeriod,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outMin,
                                                SubArray<double>^  outMax );

         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outMin,
                                                SubArray<double>^  outMax );

         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outMin,
                                                cli::array<double>^  outMax )
         {
            if( outMin == outMax ) return RetCode::BadParam;
            return MinmaxLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMin,0),
               gcnew SubArrayFrom1D<double>(outMax,0) );
         }
         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outMin,
                                                cli::array<double>^  outMax )
         {
            if( outMin == outMax ) return RetCode::BadParam;
            return MinmaxLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMin,0),
               gcnew SubArrayFrom1D<double>(outMax,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outMin,
                                                cli::array<double>^  outMax );
         static enum class RetCode MinmaxLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outMin,
                                                cli::array<double>^  outMax );
#endif

         #define TA_MINMAX Core::Minmax
         #define TA_MINMAX_Lookback Core::MinmaxLookback
         #define TA_MINMAX_Logic Core::MinmaxLogic
