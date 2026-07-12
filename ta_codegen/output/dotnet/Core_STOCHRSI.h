         static int StochrsiLookback( int           optInTimePeriod  /* From 2 to 100000 */, int           optInFastK_Period  /* From 1 to 100000 */, int           optInFastD_Period  /* From 1 to 100000 */, int           optInFastD_MAType );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outFastK,
                                             SubArray<double>^  outFastD );

         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outFastK,
                                             SubArray<double>^  outFastD );

         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return Stochrsi( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return Stochrsi( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD );
         static enum class RetCode Stochrsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int optInTimePeriod,
                                             int optInFastK_Period,
                                             int optInFastD_Period,
                                             int optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<double>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outFastK,
                                                  SubArray<double>^  outFastD );

         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<float>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outFastK,
                                                  SubArray<double>^  outFastD );

         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outFastK,
                                                  cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return StochrsiLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInTimePeriod,
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outFastK,
                                                  cli::array<double>^  outFastD )
         {
            if( outFastK == outFastD ) return RetCode::BadParam;
            return StochrsiLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInTimePeriod,
                         optInFastK_Period,
                         optInFastD_Period,
                         optInFastD_MAType,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outFastK,0),
               gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outFastK,
                                                  cli::array<double>^  outFastD );
         static enum class RetCode StochrsiLogic( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inReal,
                                                  int optInTimePeriod,
                                                  int optInFastK_Period,
                                                  int optInFastD_Period,
                                                  int optInFastD_MAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outFastK,
                                                  cli::array<double>^  outFastD );
#endif

         #define TA_STOCHRSI Core::Stochrsi
         #define TA_STOCHRSI_Lookback Core::StochrsiLookback
         #define TA_STOCHRSI_Logic Core::StochrsiLogic
