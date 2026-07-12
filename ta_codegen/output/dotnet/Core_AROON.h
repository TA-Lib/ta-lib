         static int AroonLookback( int           optInTimePeriod  /* From 2 to 100000 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inHigh,
                                          SubArray<double>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outAroonDown,
                                          SubArray<double>^  outAroonUp );

         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inHigh,
                                          SubArray<float>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outAroonDown,
                                          SubArray<double>^  outAroonUp );

         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp )
         {
            if( outAroonDown == outAroonUp ) return RetCode::BadParam;
            return Aroon( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outAroonDown,0),
               gcnew SubArrayFrom1D<double>(outAroonUp,0) );
         }
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp )
         {
            if( outAroonDown == outAroonUp ) return RetCode::BadParam;
            return Aroon( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outAroonDown,0),
               gcnew SubArrayFrom1D<double>(outAroonUp,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp );
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          int optInTimePeriod,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<double>^  outAroonDown,
                                               SubArray<double>^  outAroonUp );

         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<double>^  outAroonDown,
                                               SubArray<double>^  outAroonUp );

         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outAroonDown,
                                               cli::array<double>^  outAroonUp )
         {
            if( outAroonDown == outAroonUp ) return RetCode::BadParam;
            return AroonLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outAroonDown,0),
               gcnew SubArrayFrom1D<double>(outAroonUp,0) );
         }
         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outAroonDown,
                                               cli::array<double>^  outAroonUp )
         {
            if( outAroonDown == outAroonUp ) return RetCode::BadParam;
            return AroonLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         optInTimePeriod,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outAroonDown,0),
               gcnew SubArrayFrom1D<double>(outAroonUp,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outAroonDown,
                                               cli::array<double>^  outAroonUp );
         static enum class RetCode AroonLogic( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               int optInTimePeriod,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outAroonDown,
                                               cli::array<double>^  outAroonUp );
#endif

         #define TA_AROON Core::Aroon
         #define TA_AROON_Lookback Core::AroonLookback
         #define TA_AROON_Logic Core::AroonLogic
