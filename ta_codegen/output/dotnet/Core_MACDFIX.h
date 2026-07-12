         static int MacdfixLookback( int           optInSignalPeriod  /* From 1 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return Macdfix( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return Macdfix( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
         static enum class RetCode Macdfix( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int optInSignalPeriod,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<double>^  outMACD,
                                                 SubArray<double>^  outMACDSignal,
                                                 SubArray<double>^  outMACDHist );

         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<double>^  outMACD,
                                                 SubArray<double>^  outMACDSignal,
                                                 SubArray<double>^  outMACDHist );

         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outMACD,
                                                 cli::array<double>^  outMACDSignal,
                                                 cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return MacdfixLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outMACD,
                                                 cli::array<double>^  outMACDSignal,
                                                 cli::array<double>^  outMACDHist )
         {
            if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) return RetCode::BadParam;
            return MacdfixLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInSignalPeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMACD,0),
               gcnew SubArrayFrom1D<double>(outMACDSignal,0),
               gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outMACD,
                                                 cli::array<double>^  outMACDSignal,
                                                 cli::array<double>^  outMACDHist );
         static enum class RetCode MacdfixLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inReal,
                                                 int optInSignalPeriod,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outMACD,
                                                 cli::array<double>^  outMACDSignal,
                                                 cli::array<double>^  outMACDHist );
#endif

         #define TA_MACDFIX Core::Macdfix
         #define TA_MACDFIX_Lookback Core::MacdfixLookback
         #define TA_MACDFIX_Logic Core::MacdfixLogic
