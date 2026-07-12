         static int Ht_phasorLookback( void );

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outInPhase,
                                              SubArray<double>^  outQuadrature );

         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outInPhase,
                                              SubArray<double>^  outQuadrature );

         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outInPhase,
                                              cli::array<double>^  outQuadrature )
         {
            if( outInPhase == outQuadrature ) return RetCode::BadParam;
            return Ht_phasor( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outInPhase,0),
               gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outInPhase,
                                              cli::array<double>^  outQuadrature )
         {
            if( outInPhase == outQuadrature ) return RetCode::BadParam;
            return Ht_phasor( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outInPhase,0),
               gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outInPhase,
                                              cli::array<double>^  outQuadrature );
         static enum class RetCode Ht_phasor( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outInPhase,
                                              cli::array<double>^  outQuadrature );
#endif

#if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outInPhase,
                                                   SubArray<double>^  outQuadrature );

         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outInPhase,
                                                   SubArray<double>^  outQuadrature );

         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outInPhase,
                                                   cli::array<double>^  outQuadrature )
         {
            if( outInPhase == outQuadrature ) return RetCode::BadParam;
            return Ht_phasorLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outInPhase,0),
               gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outInPhase,
                                                   cli::array<double>^  outQuadrature )
         {
            if( outInPhase == outQuadrature ) return RetCode::BadParam;
            return Ht_phasorLogic( startIdx, endIdx,
                         gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
               gcnew SubArrayFrom1D<double>(outInPhase,0),
               gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
#elif defined( _MANAGED )
         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outInPhase,
                                                   cli::array<double>^  outQuadrature );
         static enum class RetCode Ht_phasorLogic( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outInPhase,
                                                   cli::array<double>^  outQuadrature );
#endif

         #define TA_HT_PHASOR Core::Ht_phasor
         #define TA_HT_PHASOR_Lookback Core::Ht_phasorLookback
         #define TA_HT_PHASOR_Logic Core::Ht_phasorLogic
