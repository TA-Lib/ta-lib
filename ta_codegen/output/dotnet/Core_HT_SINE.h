         static int Ht_sineLookback( void );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outSine,
                                            SubArray<double>^  outLeadSine );

         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outSine,
                                            SubArray<double>^  outLeadSine );

         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outSine,
                                            cli::array<double>^  outLeadSine )
         {
            if( outSine == outLeadSine ) return RetCode::BadParam;
            return Ht_sine( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outSine,0),
               gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outSine,
                                            cli::array<double>^  outLeadSine )
         {
            if( outSine == outLeadSine ) return RetCode::BadParam;
            return Ht_sine( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outSine,0),
               gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outSine,
                                            cli::array<double>^  outLeadSine );
         static enum class RetCode Ht_sine( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outSine,
                                            cli::array<double>^  outLeadSine );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<double>^  outSine,
                                                 SubArray<double>^  outLeadSine );

         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<double>^  outSine,
                                                 SubArray<double>^  outLeadSine );

         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outSine,
                                                 cli::array<double>^  outLeadSine )
         {
            if( outSine == outLeadSine ) return RetCode::BadParam;
            return Ht_sineLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outSine,0),
               gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outSine,
                                                 cli::array<double>^  outLeadSine )
         {
            if( outSine == outLeadSine ) return RetCode::BadParam;
            return Ht_sineLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outSine,0),
               gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outSine,
                                                 cli::array<double>^  outLeadSine );
         static enum class RetCode Ht_sineLogic( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inReal,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<double>^  outSine,
                                                 cli::array<double>^  outLeadSine );
#endif

         #define TA_HT_SINE Core::Ht_sine
         #define TA_HT_SINE_Lookback Core::Ht_sineLookback
         #define TA_HT_SINE_Logic Core::Ht_sineLogic
