         static int MamaLookback( double           optInFastLimit  /* From 0.01 to 0.99 */, double           optInSlowLimit  /* From 0.01 to 0.99 */ );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMAMA,
                                         SubArray<double>^  outFAMA );

         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMAMA,
                                         SubArray<double>^  outFAMA );

         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA )
         {
            if( outMAMA == outFAMA ) return RetCode::BadParam;
            return Mama( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInFastLimit,
                         optInSlowLimit,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMAMA,0),
               gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA )
         {
            if( outMAMA == outFAMA ) return RetCode::BadParam;
            return Mama( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInFastLimit,
                         optInSlowLimit,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMAMA,0),
               gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA );
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         double optInFastLimit,
                                         double optInSlowLimit,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outMAMA,
                                              SubArray<double>^  outFAMA );

         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outMAMA,
                                              SubArray<double>^  outFAMA );

         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMAMA,
                                              cli::array<double>^  outFAMA )
         {
            if( outMAMA == outFAMA ) return RetCode::BadParam;
            return MamaLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
                         optInFastLimit,
                         optInSlowLimit,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMAMA,0),
               gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMAMA,
                                              cli::array<double>^  outFAMA )
         {
            if( outMAMA == outFAMA ) return RetCode::BadParam;
            return MamaLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
                         optInFastLimit,
                         optInSlowLimit,
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outMAMA,0),
               gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMAMA,
                                              cli::array<double>^  outFAMA );
         static enum class RetCode MamaLogic( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              double optInFastLimit,
                                              double optInSlowLimit,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outMAMA,
                                              cli::array<double>^  outFAMA );
#endif

         #define TA_MAMA Core::Mama
         #define TA_MAMA_Lookback Core::MamaLookback
         #define TA_MAMA_Logic Core::MamaLogic
