         static int StochfLookback( int           optInFastK_Period  /* From 1 to 100000 */, int           optInFastD_Period  /* From 1 to 100000 */, int           optInFastD_MAType );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           SubArray<double>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outFastK,
                                           SubArray<double>^  outFastD );

         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           SubArray<float>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outFastK,
                                           SubArray<double>^  outFastD );

         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return Stochf( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return Stochf( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD );
         static enum class RetCode Stochf( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int optInFastK_Period,
                                           int optInFastD_Period,
                                           int optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outFastK,
                                                SubArray<double>^  outFastD );

         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outFastK,
                                                SubArray<double>^  outFastD );

         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outFastK,
                                                cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return StochfLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outFastK,
                                                cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return StochfLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outFastK,
                                                cli::array<double>^  outFastD );
         static enum class RetCode StochfLogic( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                int optInFastK_Period,
                                                int optInFastD_Period,
                                                int optInFastD_MAType,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outFastK,
                                                cli::array<double>^  outFastD );
#endif

         #define TA_STOCHF Core::Stochf
         #define TA_STOCHF_Lookback Core::StochfLookback
         #define TA_STOCHF_Logic Core::StochfLogic
