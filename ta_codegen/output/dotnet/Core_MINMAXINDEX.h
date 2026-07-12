         static int MinmaxindexLookback( int           optInTimePeriod  /* From 2 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outMinIdx,
                                                SubArray<int>^  outMaxIdx );

         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outMinIdx,
                                                SubArray<int>^  outMaxIdx );

         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx )
         {
            if( outMinIdx == outMaxIdx ) return RetCode::BadParam;
            return Minmaxindex( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<int>(outMinIdx,0),
               gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx )
         {
            if( outMinIdx == outMaxIdx ) return RetCode::BadParam;
            return Minmaxindex( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<int>(outMinIdx,0),
               gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx );
         static enum class RetCode Minmaxindex( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int optInTimePeriod,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outMinIdx,
                                                     SubArray<int>^  outMaxIdx );

         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outMinIdx,
                                                     SubArray<int>^  outMaxIdx );

         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outMinIdx,
                                                     cli::array<int>^  outMaxIdx )
         {
            if( outMinIdx == outMaxIdx ) return RetCode::BadParam;
            return MinmaxindexLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<int>(outMinIdx,0),
               gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outMinIdx,
                                                     cli::array<int>^  outMaxIdx )
         {
            if( outMinIdx == outMaxIdx ) return RetCode::BadParam;
            return MinmaxindexLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<int>(outMinIdx,0),
               gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outMinIdx,
                                                     cli::array<int>^  outMaxIdx );
         static enum class RetCode MinmaxindexLogic( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inReal,
                                                     int optInTimePeriod,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outMinIdx,
                                                     cli::array<int>^  outMaxIdx );
#endif

         #define TA_MINMAXINDEX Core::Minmaxindex
         #define TA_MINMAXINDEX_Lookback Core::MinmaxindexLookback
         #define TA_MINMAXINDEX_Logic Core::MinmaxindexLogic
