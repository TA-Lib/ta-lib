         static int MacdLookback( int           optInFastPeriod  /* From 2 to 100000 */, int           optInSlowPeriod  /* From 2 to 100000 */, int           optInSignalPeriod  /* From 1 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMACD,
                                         SubArray<double>^  outMACDSignal,
                                         SubArray<double>^  outMACDHist );

         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMACD,
                                         SubArray<double>^  outMACDSignal,
                                         SubArray<double>^  outMACDHist );

         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return Macd( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInFastPeriod,
                         optInSlowPeriod,
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return Macd( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInFastPeriod,
                         optInSlowPeriod,
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist );
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int optInFastPeriod,
                                         int optInSlowPeriod,
                                         int optInSignalPeriod,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outMACD,
                                              SubArray<double>^  outMACDSignal,
                                              SubArray<double>^  outMACDHist );

         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outMACD,
                                              SubArray<double>^  outMACDSignal,
                                              SubArray<double>^  outMACDHist );

         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMACD,
                                              cli::array<double>^  outMACDSignal,
                                              cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return MacdLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInFastPeriod,
                         optInSlowPeriod,
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMACD,
                                              cli::array<double>^  outMACDSignal,
                                              cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return MacdLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInFastPeriod,
                         optInSlowPeriod,
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMACD,
                                              cli::array<double>^  outMACDSignal,
                                              cli::array<double>^  outMACDHist );
         static enum class RetCode MacdLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              int optInFastPeriod,
                                              int optInSlowPeriod,
                                              int optInSignalPeriod,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMACD,
                                              cli::array<double>^  outMACDSignal,
                                              cli::array<double>^  outMACDHist );
#endif

         #define TA_MACD Core::Macd
         #define TA_MACD_Lookback Core::MacdLookback
         #define TA_MACD_Logic Core::MacdLogic
