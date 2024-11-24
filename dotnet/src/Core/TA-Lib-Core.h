/* TA-LIB Copyright (c) 1999-2008, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PSG      Przemyslaw Grodzki (pgrodzki@ki.net.pl)
 *  RM       Robert Meier (talib@meierlim.com http://www.meierlim.com)
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  050703 MF     First version with all the TA functions.
 *  112304 PSG    Fix #1072276 for TA_CandleDefaultSettings size.
 *  123004 RM,MF  Adapt code to work with Visual Studio 2005
 *  112605 MF     New TA_BOP (Balance Of Power) function.
 */

#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

namespace TicTacTec
{
	namespace TA
	{
	   namespace Library
	   {
          #if defined( USE_SUBARRAY )

		   // Abstration of a single dimension array.
		   //		   
		   // The concrete implementation might be a sub-section of 
		   // a 1D or 2D array and can even be of a different
		   // value type (e.g. integer).
		   generic<typename T> public ref class SubArray abstract
           {
		   public:

			   // Array like interface.
			   property T default[int] { 
		          public:
					  virtual T get(int) abstract; 
					  virtual void set(int,T) abstract; 
			   }

			   static void Copy(SubArray<T>^ source, int sourceIndex, SubArray<T>^ dest, int destIndex, int length )
			   {
				   for( int i=0; i < length; i++, sourceIndex++, destIndex++ )
				   {
					   dest[destIndex] = source[sourceIndex];
				   }
			   }
           };

		   generic<typename T> public ref class SubArrayFrom1D : public SubArray<T>
		   {
		   public:
				SubArrayFrom1D<T>( cli::array<T>^ dataArray, int offset )
				{
				   mDataArray = dataArray;
				   mOffset = offset;
				}

			   SubArrayFrom1D<T>() {}
			   SubArrayFrom1D<T>(const SubArrayFrom1D<T>^) {}

			    property T default[int]
				{
				public:
					virtual T get(int offset) override 
					{
						return mDataArray[mOffset+offset];
					}    
					virtual void set(int offset, T value ) override
					{
						mDataArray[mOffset+offset] = value;
					} 					
				}

			private:
				int mOffset;
			    cli::array<T>^ mDataArray;
		   };

		   // Allows to access a 2D array as a SubArray.
		   //
		   // One of the dimension is made fix at construction time.
		   //
		   // The dimension provided in the constructor is 'indice2'
		   // when accessing the array as [indice1,indice2].           
		   generic<typename T> public ref class SubArrayFrom2D : public SubArray<T>
		   {
		   public:								
				SubArrayFrom2D<T>( cli::array<T,2>^ dataArray, int offset, int dimension )
				{
				   mDataArray = dataArray;
				   mOffset = offset;
				   mDimension  = dimension;
				}

			   SubArrayFrom2D<T>() {}
			   SubArrayFrom2D<T>(const SubArrayFrom2D<T>^) {}

			    property T default[int]
				{
				public:
					virtual T get(int offset) override
					{
						return mDataArray[mOffset+offset,mDimension];
					}    
					virtual void set(int offset, T value ) override
					{
						mDataArray[mOffset+offset,mDimension] = value;
					} 					
				}

		   private:
				int mOffset;
				int mDimension;
                cli::array<T,2>^ mDataArray;
		   };

           // Allows to transform any 2D array of objects into a SubArray<T>.		   
/*
		   generic<typename T> where T:SubArray<System::Object^>
		   public ref class SubArrayToDouble : public SubArray<double>
		   {
           public:
			    SubArrayToDouble<T>( T subArray )
				{
					mSubArray = subArray;
				}

			    SubArrayToDouble<T>() {}
			    SubArrayToDouble<T>(const SubArrayToDouble<T>^) {}

			    property double default[int]
				{
				public:
					virtual double get(int offset) override 
					{
						return mSubArray[offset];
					}    
					virtual void set(int offset, double value ) override
					{
						mSubArray[offset] = safe_cast<System::Object>(value);
					} 					
				}

			private:
				T mSubArray;				
		   };*/

//where T:SubArray<System::Object^>
		   generic<typename T>  public ref class SubArrayFrom2DObject : public SubArray<T>		   
		   {
	       public:
			    SubArrayFrom2DObject<T>( cli::array<System::Object^,2>^ dataArray, int offset, int dimension )
				{
				   mDataArray = dataArray;
				   mOffset = offset;
				   mDimension  = dimension;
				}

			   SubArrayFrom2DObject<T>() {}
			   SubArrayFrom2DObject<T>(const SubArrayFrom2DObject<T>^) {}

			    property T default[int]
				{
				public:
					virtual T get(int offset) override
					{
						return safe_cast<T>(mDataArray[mOffset+offset,mDimension]);
					}    
					virtual void set(int offset, T value ) override
					{
						mDataArray[mOffset+offset,mDimension] = (T)value;
					} 					
				}

		   private:
				int mOffset;
				int mDimension;                
				cli::array<System::Object^,2>^ mDataArray;
		   };


		   generic<typename T>  public ref class SubArrayFrom1DObject : public SubArray<T>		   
		   {
	       public:
			    SubArrayFrom1DObject<T>( cli::array<System::Object^>^ dataArray, int offset )
				{
				   mDataArray = dataArray;
				   mOffset = offset;				   
				}

			   SubArrayFrom1DObject<T>() {}
			   SubArrayFrom1DObject<T>(const SubArrayFrom1DObject<T>^) {}

			    property T default[int]
				{
				public:
					virtual T get(int offset) override
					{
						return safe_cast<T>(mDataArray[mOffset+offset]);
					}    
					virtual void set(int offset, T value ) override
					{
						mDataArray[mOffset+offset] = (T)value;
					} 					
				}

		   private:
				int mOffset;				
				cli::array<System::Object^>^ mDataArray;
		   };


		   // Allows to transform a SubArrayFrom2D<Object^> into a SubArray<double> 
		   // without copy (cast on the fly).
/*
		   public ref class SubArrayFrom2DObjectToDouble : public SubArray<double>
		   {
		   public:
			   SubArrayFrom2DObjectToDouble( SubArrayFrom2D<System::Object^>^ subArray )
				{
					mSubArray = subArray;
				}

			    SubArrayFrom2DObjectToDouble() {}
			    SubArrayFrom2DObjectToDouble(const SubArrayFrom2DObjectToDouble^) {}

			    property double default[int]
				{
				public:
					virtual double get(int offset) override 
					{
						return (double)mSubArray[offset];
					}    
					virtual void set(int offset, double value ) override
					{
						mSubArray[offset] = (double)value;
					} 					
				}

			private:
				SubArrayFrom2D<System::Object^>^ mSubArray;				
		   };*/

		   // Allows to transform a SubArray<float> into a SubArray<double> without copy
		   // (cast on the fly).
		   public ref class SubArrayFloatToDouble : public SubArray<double>
		   {
		   public:
				SubArrayFloatToDouble( SubArray<float>^ subArray )
				{
					mSubArray = subArray;
				}

			    SubArrayFloatToDouble() {}
			    SubArrayFloatToDouble(const SubArrayFloatToDouble^) {}

			    property double default[int]
				{
				public:
					virtual double get(int offset) override 
					{
						return (double)mSubArray[offset];
					}    
					virtual void set(int offset, double value ) override
					{
						mSubArray[offset] = (float)value;
					} 					
				}

			private:
				SubArray<float>^ mSubArray;				
		   };

          #endif

		  public ref class Core abstract sealed
		  {
		  public:
			 #include "ta_defs.h"


		  private:
			  ref class CandleSetting sealed 
			  {
			  public:
				  enum class CandleSettingType settingType;
				  enum class RangeType rangeType;
				  int     avgPeriod;
				  double  factor;
			 };

			 ref class GlobalsType sealed
			 {
			 public:
				GlobalsType()
				{
					unstablePeriod = gcnew cli::array<unsigned int>((int)FuncUnstId::FuncUnstAll);
					compatibility = Compatibility::Default;
					for( int i=0; i < (int)FuncUnstId::FuncUnstAll; i++ )
						unstablePeriod[i] = 0;
					candleSettings = gcnew cli::array<CandleSetting^>((int)CandleSettingType::AllCandleSettings);
					for( int j=0; j < candleSettings->Length; j++ )
					{
						candleSettings[j] = gcnew CandleSetting();					
					}
				}

				/* For handling the compatibility with other software */
				Compatibility compatibility;

				/* For handling the unstable period of some TA function. */
				cli::array<unsigned int>^ unstablePeriod;

				/* For handling the candlestick global settings */
				cli::array<CandleSetting^>^ candleSettings;
			 };

			 static GlobalsType^ Globals;

             #if defined( USE_SUBARRAY )
			 static  enum class RetCode TA_INT_EMA( int           startIdx,
									int           endIdx,
									SubArray<double>^ inReal_0,
									int           optInTimePeriod_0,
									double        optInK_1,
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									SubArray<double>^ outReal_0);

			 static  enum class RetCode TA_INT_EMA( int           startIdx,
									int           endIdx,
									SubArray<float>^ inReal_0,
									int           optInTimePeriod_0,
									double        optInK_1,
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									SubArray<double>^ outReal_0)
			 {
                   return TA_INT_EMA(  startIdx,
						               endIdx,
						               gcnew SubArrayFloatToDouble(inReal_0),
						               optInTimePeriod_0,
							           optInK_1,
							           outBegIdx,
							           outNbElement,
							           outReal_0 );
			 }


             #else
			 static  enum class RetCode TA_INT_EMA( int           startIdx,
									int           endIdx,
									cli::array<double>^ inReal_0,
									int           optInTimePeriod_0, 
									double        optInK_1,
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0);

			 static  enum class RetCode TA_INT_EMA( int           startIdx,
									int           endIdx,
									cli::array<float>^ inReal_0,
									int           optInTimePeriod_0, 
									double        optInK_1,
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0);
             #endif


             #if defined( USE_SUBARRAY )
             static  enum class RetCode TA_INT_SMA( int           startIdx,
					                int           endIdx,
					                SubArray<double>^ inReal_0,
					                int           optInTimePeriod_0,
					                [Out]int% outBegIdx,
					                [Out]int% outNbElement,
					                SubArray<double>^ outReal_0);

             static  enum class RetCode TA_INT_SMA( int           startIdx,
					                int           endIdx,
					                SubArray<float>^ inReal_0,
					                int           optInTimePeriod_0,
					                [Out]int% outBegIdx,
					                [Out]int% outNbElement,
					                SubArray<double>^ outReal_0)
			 {
                 return TA_INT_SMA( startIdx,
					                endIdx,
					                gcnew SubArrayFloatToDouble(inReal_0),
					                optInTimePeriod_0,
					                outBegIdx,
					                outNbElement,
					                outReal_0);

			 }
             #else
			 static  enum class RetCode TA_INT_SMA( int     startIdx,
									int     endIdx,
									cli::array<double>^ inReal_0,
									int     optInTimePeriod_0, 
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0);
			 static  enum class RetCode TA_INT_SMA( int     startIdx,
									int     endIdx,
									cli::array<float>^ inReal_0,
									int     optInTimePeriod_0, 
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0 );
             #endif


             #if defined( USE_SUBARRAY )
             static  enum class RetCode TA_INT_MACD( int           startIdx,
					                 int           endIdx,
					                 SubArray<double>^ inReal_0,
									 int    optInFastPeriod_0, /* 0 is fix 12 */
									 int    optInSlowPeriod_1, /* 0 is fix 26 */
									 int    optInSignalPeriod_2,
					                 [Out]int% outBegIdx,
					                 [Out]int% outNbElement,
									 SubArray<double>^ outMACD_0,
									 SubArray<double>^ outMACDSignal_1,
									 SubArray<double>^ outMACDHist_2 );

             static  enum class RetCode TA_INT_MACD( int           startIdx,
					                 int           endIdx,
					                 SubArray<float>^ inReal_0,
									 int    optInFastPeriod_0, /* 0 is fix 12 */
									 int    optInSlowPeriod_1, /* 0 is fix 26 */
									 int    optInSignalPeriod_2,
					                 [Out]int% outBegIdx,
					                 [Out]int% outNbElement,
									 SubArray<double>^ outMACD_0,
									 SubArray<double>^ outMACDSignal_1,
									 SubArray<double>^ outMACDHist_2 )
			 {
				 return TA_INT_MACD( startIdx,
					                 endIdx,
					                 gcnew SubArrayFloatToDouble(inReal_0),
									 optInFastPeriod_0, /* 0 is fix 12 */
									 optInSlowPeriod_1, /* 0 is fix 26 */
									 optInSignalPeriod_2,
					                 outBegIdx,
					                 outNbElement,
									 outMACD_0,
									 outMACDSignal_1,
									 outMACDHist_2 );
			 }
             #else

			 static  enum class RetCode TA_INT_MACD( int    startIdx,
									 int    endIdx,
									 cli::array<double>^ inReal_0,
									 int    optInFastPeriod_0, /* 0 is fix 12 */
									 int    optInSlowPeriod_1, /* 0 is fix 26 */
									 int    optInSignalPeriod_2, 
									 [Out]int% outBegIdx,
									 [Out]int% outNbElement,
									 cli::array<double>^ outMACD_0,
									 cli::array<double>^ outMACDSignal_1,
									 cli::array<double>^ outMACDHist_2 );
			 static  enum class RetCode TA_INT_MACD( int    startIdx,
									 int    endIdx,
									 cli::array<float>^ inReal_0,
									 int    optInFastPeriod_0, /* 0 is fix 12 */
									 int    optInSlowPeriod_1, /* 0 is fix 26 */
									 int    optInSignalPeriod_2, 
									 [Out]int% outBegIdx,
									 [Out]int% outNbElement,
									 cli::array<double>^ outMACD_0,
									 cli::array<double>^ outMACDSignal_1,
									 cli::array<double>^ outMACDHist_2 );
             #endif

             #if defined( USE_SUBARRAY )
			 static  enum class RetCode TA_INT_PO( int    startIdx,
								   int    endIdx,
								   SubArray<double>^ inReal_0,
								   int    optInFastPeriod_0, 
								   int    optInSlowPeriod_1, 
								   MAType optInMethod_2,
								   [Out]int% outBegIdx,
								   [Out]int% outNbElement,
								   SubArray<double>^ outReal_0,
								   SubArray<double>^ tempBuffer,
								   int  doPercentageOutput );

			 static  enum class RetCode TA_INT_PO( int    startIdx,
								   int    endIdx,
								   SubArray<float>^ inReal_0,
								   int    optInFastPeriod_0, 
								   int    optInSlowPeriod_1, 
								   MAType optInMethod_2,
								   [Out]int% outBegIdx,
								   [Out]int% outNbElement,
								   SubArray<double>^ outReal_0,
								   SubArray<double>^ tempBuffer,
								   int  doPercentageOutput )
			 {
                 return TA_INT_PO( startIdx,
								   endIdx,
								   gcnew SubArrayFloatToDouble(inReal_0),
								   optInFastPeriod_0, 
								   optInSlowPeriod_1, 
								   optInMethod_2,
								   outBegIdx,
								   outNbElement,
								   outReal_0,
								   tempBuffer,
								   doPercentageOutput );
			 }

             #else
			 static  enum class RetCode TA_INT_PO( int    startIdx,
								   int    endIdx,
								   cli::array<double>^ inReal_0,
								   int    optInFastPeriod_0, 
								   int    optInSlowPeriod_1, 
								   MAType optInMethod_2,
								   [Out]int% outBegIdx,
								   [Out]int% outNbElement,
								   cli::array<double>^ outReal_0,
								   cli::array<double>^ tempBuffer,
								   int  doPercentageOutput );
			 static  enum class RetCode TA_INT_PO( int    startIdx,
								   int    endIdx,
								   cli::array<float>^ inReal_0,
								   int    optInFastPeriod_0, 
								   int    optInSlowPeriod_1, 
								   MAType optInMethod_2,
								   [Out]int% outBegIdx,
								   [Out]int% outNbElement,
								   cli::array<double>^ outReal_0,
								   cli::array<double>^ tempBuffer,
								   int  doPercentageOutput );
             #endif

             #if defined( USE_SUBARRAY )
			 static  enum class RetCode TA_INT_VAR( int    startIdx,
									int    endIdx,
									SubArray<double>^ inReal_0,
									int    optInTimePeriod_0,                       
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									SubArray<double>^ outReal_0);

			 static  enum class RetCode TA_INT_VAR( int    startIdx,
									int    endIdx,
									SubArray<float>^ inReal_0,
									int    optInTimePeriod_0,                       
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									SubArray<double>^ outReal_0)
			 {
				 return TA_INT_VAR( startIdx,
									endIdx,
									gcnew SubArrayFloatToDouble(inReal_0),
									optInTimePeriod_0,                       
									outBegIdx,
									outNbElement,
									outReal_0);
			 }
             #else
			 static  enum class RetCode TA_INT_VAR( int    startIdx,
									int    endIdx,
									cli::array<double>^ inReal_0,
									int    optInTimePeriod_0,                       
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0);
			 static  enum class RetCode TA_INT_VAR( int    startIdx,
									int    endIdx,
									cli::array<float>^ inReal_0,
									int    optInTimePeriod_0,                       
									[Out]int% outBegIdx,
									[Out]int% outNbElement,
									cli::array<double>^ outReal_0);
             #endif



             #if defined( USE_SUBARRAY )
			 static void TA_INT_stddev_using_precalc_ma( SubArray<double>^  inReal,
												  SubArray<double>^ inMovAvg,
												  int inMovAvgBegIdx,
												  int inMovAvgNbElement,
												  int timePeriod,
												  SubArray<double>^ output );

			 static void TA_INT_stddev_using_precalc_ma( SubArray<float>^  inReal,
												  SubArray<double>^ inMovAvg,
												  int inMovAvgBegIdx,
												  int inMovAvgNbElement,
												  int timePeriod,
												  SubArray<double>^ output )
			 {
				 return TA_INT_stddev_using_precalc_ma( gcnew SubArrayFloatToDouble(inReal),
												  inMovAvg,
												  inMovAvgBegIdx,
												  inMovAvgNbElement,
												  timePeriod,
												  output );
			 }
             #else
			 static void TA_INT_stddev_using_precalc_ma( cli::array<double>^  inReal,
												  cli::array<double>^ inMovAvg,
												  int inMovAvgBegIdx,
												  int inMovAvgNbElement,
												  int timePeriod,
												  cli::array<double>^ output );
			 static void TA_INT_stddev_using_precalc_ma( cli::array<float>^ inReal,
												  cli::array<double>^ inMovAvg,
												  int inMovAvgBegIdx,
												  int inMovAvgNbElement,
												  int timePeriod,
												  cli::array<double>^ output);
             #endif


		  public:
			 static Core()
			 {
				// Initialize global settings
				Globals = gcnew GlobalsType;
				/* Set the default value to global variables */
				RestoreCandleDefaultSettings( CandleSettingType::AllCandleSettings );
			 }

			 static  enum class RetCode SetUnstablePeriod(  enum class FuncUnstId id,
															   unsigned int  unstablePeriod );

			 static unsigned int GetUnstablePeriod( FuncUnstId id );

			 static  enum class RetCode SetCompatibility( enum class Compatibility value );
			 static  enum class Compatibility GetCompatibility( void );

			 static  enum class RetCode SetCandleSettings(  enum class CandleSettingType settingType, 
															enum class RangeType rangeType,
															int avgPeriod, 
															double factor )
			 {    
				 if( settingType >= CandleSettingType::AllCandleSettings )
				 {
					 return RetCode::BadParam;
				 }
				 Globals->candleSettings[(int)settingType]->settingType = settingType;
				 Globals->candleSettings[(int)settingType]->rangeType = rangeType;
				 Globals->candleSettings[(int)settingType]->avgPeriod = avgPeriod;
				 Globals->candleSettings[(int)settingType]->factor = factor;
				 return RetCode::Success;
			 }

			 
			 static enum class RetCode RestoreCandleDefaultSettings( CandleSettingType settingType )
			 {			
				switch( settingType )
				{
				case CandleSettingType::BodyLong:
				   SetCandleSettings( CandleSettingType::BodyLong, RangeType::RealBody, 10, 1.0 );
				   break;
				case CandleSettingType::BodyVeryLong:
				   SetCandleSettings( CandleSettingType::BodyVeryLong, RangeType::RealBody, 10, 3.0 );
				   break;
				case CandleSettingType::BodyShort:
				   SetCandleSettings( CandleSettingType::BodyShort, RangeType::RealBody, 10, 1.0 );
				   break;
				case CandleSettingType::BodyDoji:
				   SetCandleSettings( CandleSettingType::BodyDoji, RangeType::HighLow, 10, 0.1 );
				   break;
				case CandleSettingType::ShadowLong:
				   SetCandleSettings( CandleSettingType::ShadowLong, RangeType::RealBody, 0, 1.0 );
				   break;
				case CandleSettingType::ShadowVeryLong:
				   SetCandleSettings( CandleSettingType::ShadowVeryLong, RangeType::RealBody, 0, 2.0 );
				   break;
				case CandleSettingType::ShadowShort:
				   SetCandleSettings( CandleSettingType::ShadowShort, RangeType::Shadows, 10, 1.0 );
				   break;
				case CandleSettingType::ShadowVeryShort:
				   SetCandleSettings( CandleSettingType::ShadowVeryShort, RangeType::HighLow, 10, 0.1 );
				   break;
				case CandleSettingType::Near:
				   SetCandleSettings( CandleSettingType::Near, RangeType::HighLow, 5, 0.2 );
				   break;
				case CandleSettingType::Far:
				   SetCandleSettings( CandleSettingType::Far, RangeType::HighLow, 5, 0.6 );
				   break;
				case CandleSettingType::Equal:
				   SetCandleSettings( CandleSettingType::Equal, RangeType::HighLow, 5, 0.05);
				   break;
				case CandleSettingType::AllCandleSettings:
				   SetCandleSettings( CandleSettingType::BodyLong, RangeType::RealBody, 10, 1.0 );
				   SetCandleSettings( CandleSettingType::BodyVeryLong, RangeType::RealBody, 10, 3.0 );
				   SetCandleSettings( CandleSettingType::BodyShort, RangeType::RealBody, 10, 1.0 );
				   SetCandleSettings( CandleSettingType::BodyDoji, RangeType::HighLow, 10, 0.1 );
				   SetCandleSettings( CandleSettingType::ShadowLong, RangeType::RealBody, 0, 1.0 );
				   SetCandleSettings( CandleSettingType::ShadowVeryLong, RangeType::RealBody, 0, 2.0 );
				   SetCandleSettings( CandleSettingType::ShadowShort, RangeType::Shadows, 10, 1.0 );
				   SetCandleSettings( CandleSettingType::ShadowVeryShort, RangeType::HighLow, 10, 0.1 );
				   SetCandleSettings( CandleSettingType::Near, RangeType::HighLow, 5, 0.2 );
				   SetCandleSettings( CandleSettingType::Far, RangeType::HighLow, 5, 0.6 );
				   SetCandleSettings( CandleSettingType::Equal, RangeType::HighLow, 5, 0.05);
				   break;
				}

				return RetCode::Success;
			 }

/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
         static int AccbandsLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             SubArray<double>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outRealUpperBand,
                                             SubArray<double>^  outRealMiddleBand,
                                             SubArray<double>^  outRealLowerBand );

         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             SubArray<float>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outRealUpperBand,
                                             SubArray<double>^  outRealMiddleBand,
                                             SubArray<double>^  outRealLowerBand );

         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand )
         { return Accbands( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 gcnew SubArrayFrom1D<double>(inClose,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
                   gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
                   gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand )
         { return Accbands( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 gcnew SubArrayFrom1D<float>(inClose,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
                   gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
                   gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand );
         static enum class RetCode Accbands( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outRealUpperBand,
                                             cli::array<double>^  outRealMiddleBand,
                                             cli::array<double>^  outRealLowerBand );
         #endif

         #define TA_ACCBANDS Core::Accbands
         #define TA_ACCBANDS_Lookback Core::AccbandsLookback

         static int AcosLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Acos( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Acos( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Acos( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ACOS Core::Acos
         #define TA_ACOS_Lookback Core::AcosLookback

         static int AdLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       SubArray<double>^ inHigh,
                                       SubArray<double>^ inLow,
                                       SubArray<double>^ inClose,
                                       SubArray<double>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       SubArray<float>^ inHigh,
                                       SubArray<float>^ inLow,
                                       SubArray<float>^ inClose,
                                       SubArray<float>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inHigh,
                                       cli::array<double>^ inLow,
                                       cli::array<double>^ inClose,
                                       cli::array<double>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Ad( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              gcnew SubArrayFrom1D<double>(inVolume,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inHigh,
                                       cli::array<float>^ inLow,
                                       cli::array<float>^ inClose,
                                       cli::array<float>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Ad( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              gcnew SubArrayFrom1D<float>(inVolume,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inHigh,
                                       cli::array<double>^ inLow,
                                       cli::array<double>^ inClose,
                                       cli::array<double>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         static enum class RetCode Ad( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inHigh,
                                       cli::array<float>^ inLow,
                                       cli::array<float>^ inClose,
                                       cli::array<float>^ inVolume,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         #endif

         #define TA_AD Core::Ad
         #define TA_AD_Lookback Core::AdLookback

         static int AddLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal0,
                                        SubArray<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal0,
                                        SubArray<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Add( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal0,0),
                          gcnew SubArrayFrom1D<double>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Add( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal0,0),
                          gcnew SubArrayFrom1D<float>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Add( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_ADD Core::Add
         #define TA_ADD_Lookback Core::AddLookback

         static int AdOscLookback( int           optInFastPeriod, /* From 2 to 100000 */
                                 int           optInSlowPeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inHigh,
                                          SubArray<double>^ inLow,
                                          SubArray<double>^ inClose,
                                          SubArray<double>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inHigh,
                                          SubArray<float>^ inLow,
                                          SubArray<float>^ inClose,
                                          SubArray<float>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          cli::array<double>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return AdOsc( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              gcnew SubArrayFrom1D<double>(inVolume,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          cli::array<float>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return AdOsc( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              gcnew SubArrayFrom1D<float>(inVolume,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          cli::array<double>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         static enum class RetCode AdOsc( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          cli::array<float>^ inVolume,
                                          int           optInFastPeriod, /* From 2 to 100000 */
                                          int           optInSlowPeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         #endif

         #define TA_ADOSC Core::AdOsc
         #define TA_ADOSC_Lookback Core::AdOscLookback

         static int AdxLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        SubArray<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        SubArray<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Adx( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Adx( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Adx( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_ADX Core::Adx
         #define TA_ADX_Lookback Core::AdxLookback

         static int AdxrLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inHigh,
                                         SubArray<double>^ inLow,
                                         SubArray<double>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inHigh,
                                         SubArray<float>^ inLow,
                                         SubArray<float>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inHigh,
                                         cli::array<double>^ inLow,
                                         cli::array<double>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Adxr( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inHigh,
                                         cli::array<float>^ inLow,
                                         cli::array<float>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Adxr( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inHigh,
                                         cli::array<double>^ inLow,
                                         cli::array<double>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Adxr( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inHigh,
                                         cli::array<float>^ inLow,
                                         cli::array<float>^ inClose,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ADXR Core::Adxr
         #define TA_ADXR_Lookback Core::AdxrLookback

         static int ApoLookback( int           optInFastPeriod, /* From 2 to 100000 */
                               int           optInSlowPeriod, /* From 2 to 100000 */
                               MAType        optInMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Apo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInMAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Apo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInMAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Apo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_APO Core::Apo
         #define TA_APO_Lookback Core::ApoLookback

         static int AroonLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inHigh,
                                          SubArray<double>^ inLow,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outAroonDown,
                                          SubArray<double>^  outAroonUp );

         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inHigh,
                                          SubArray<float>^ inLow,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outAroonDown,
                                          SubArray<double>^  outAroonUp );

         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp )
         { return Aroon( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outAroonDown,0),
                gcnew SubArrayFrom1D<double>(outAroonUp,0) );
         }
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp )
         { return Aroon( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              optInTimePeriod, /* From 2 to 100000 */
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
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp );
         static enum class RetCode Aroon( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outAroonDown,
                                          cli::array<double>^  outAroonUp );
         #endif

         #define TA_AROON Core::Aroon
         #define TA_AROON_Lookback Core::AroonLookback

         static int AroonOscLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return AroonOsc( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return AroonOsc( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode AroonOsc( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_AROONOSC Core::AroonOsc
         #define TA_AROONOSC_Lookback Core::AroonOscLookback

         static int AsinLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Asin( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Asin( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Asin( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ASIN Core::Asin
         #define TA_ASIN_Lookback Core::AsinLookback

         static int AtanLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Atan( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Atan( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Atan( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ATAN Core::Atan
         #define TA_ATAN_Lookback Core::AtanLookback

         static int AtrLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        SubArray<double>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        SubArray<float>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Atr( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Atr( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Atr( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_ATR Core::Atr
         #define TA_ATR_Lookback Core::AtrLookback

         static int AvgPriceLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inOpen,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             SubArray<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inOpen,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             SubArray<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inOpen,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return AvgPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inOpen,0),
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 gcnew SubArrayFrom1D<double>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inOpen,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return AvgPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inOpen,0),
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 gcnew SubArrayFrom1D<float>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inOpen,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode AvgPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inOpen,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_AVGPRICE Core::AvgPrice
         #define TA_AVGPRICE_Lookback Core::AvgPriceLookback

         static int AvgDevLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return AvgDev( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return AvgDev( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode AvgDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_AVGDEV Core::AvgDev
         #define TA_AVGDEV_Lookback Core::AvgDevLookback

         static int BbandsLookback( int           optInTimePeriod, /* From 2 to 100000 */
                                  double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                  double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                  MAType        optInMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outRealUpperBand,
                                           SubArray<double>^  outRealMiddleBand,
                                           SubArray<double>^  outRealLowerBand );

         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outRealUpperBand,
                                           SubArray<double>^  outRealMiddleBand,
                                           SubArray<double>^  outRealLowerBand );

         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand )
         { return Bbands( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
               optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInMAType,
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
                 gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
                 gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand )
         { return Bbands( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
               optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInMAType,
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outRealUpperBand,0),
                 gcnew SubArrayFrom1D<double>(outRealMiddleBand,0),
                 gcnew SubArrayFrom1D<double>(outRealLowerBand,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand );
         static enum class RetCode Bbands( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDevUp, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInNbDevDn, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           MAType        optInMAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outRealUpperBand,
                                           cli::array<double>^  outRealMiddleBand,
                                           cli::array<double>^  outRealLowerBand );
         #endif

         #define TA_BBANDS Core::Bbands
         #define TA_BBANDS_Lookback Core::BbandsLookback

         static int BetaLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal0,
                                         SubArray<double>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal0,
                                         SubArray<float>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal0,
                                         cli::array<double>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Beta( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal0,0),
                          gcnew SubArrayFrom1D<double>(inReal1,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal0,
                                         cli::array<float>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Beta( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal0,0),
                          gcnew SubArrayFrom1D<float>(inReal1,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal0,
                                         cli::array<double>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Beta( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal0,
                                         cli::array<float>^ inReal1,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_BETA Core::Beta
         #define TA_BETA_Lookback Core::BetaLookback

         static int BopLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inOpen,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        SubArray<double>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inOpen,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        SubArray<float>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inOpen,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Bop( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inOpen,0),
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inOpen,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Bop( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inOpen,0),
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inOpen,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Bop( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inOpen,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_BOP Core::Bop
         #define TA_BOP_Lookback Core::BopLookback

         static int CciLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        SubArray<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        SubArray<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cci( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cci( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Cci( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_CCI Core::Cci
         #define TA_CCI_Lookback Core::CciLookback

         static int Cdl2CrowsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return Cdl2Crows( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return Cdl2Crows( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode Cdl2Crows( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDL2CROWS Core::Cdl2Crows
         #define TA_CDL2CROWS_Lookback Core::Cdl2CrowsLookback

         static int Cdl3BlackCrowsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return Cdl3BlackCrows( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return Cdl3BlackCrows( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode Cdl3BlackCrows( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3BLACKCROWS Core::Cdl3BlackCrows
         #define TA_CDL3BLACKCROWS_Lookback Core::Cdl3BlackCrowsLookback

         static int Cdl3InsideLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inOpen,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               SubArray<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inOpen,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               SubArray<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return Cdl3Inside( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<double>(inOpen,0),
                   gcnew SubArrayFrom1D<double>(inHigh,0),
                   gcnew SubArrayFrom1D<double>(inLow,0),
                   gcnew SubArrayFrom1D<double>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return Cdl3Inside( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<float>(inOpen,0),
                   gcnew SubArrayFrom1D<float>(inHigh,0),
                   gcnew SubArrayFrom1D<float>(inLow,0),
                   gcnew SubArrayFrom1D<float>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         static enum class RetCode Cdl3Inside( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3INSIDE Core::Cdl3Inside
         #define TA_CDL3INSIDE_Lookback Core::Cdl3InsideLookback

         static int Cdl3LineStrikeLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return Cdl3LineStrike( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return Cdl3LineStrike( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode Cdl3LineStrike( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3LINESTRIKE Core::Cdl3LineStrike
         #define TA_CDL3LINESTRIKE_Lookback Core::Cdl3LineStrikeLookback

         static int Cdl3OutsideLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return Cdl3Outside( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return Cdl3Outside( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode Cdl3Outside( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3OUTSIDE Core::Cdl3Outside
         #define TA_CDL3OUTSIDE_Lookback Core::Cdl3OutsideLookback

         static int Cdl3StarsInSouthLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inOpen,
                                                     SubArray<double>^ inHigh,
                                                     SubArray<double>^ inLow,
                                                     SubArray<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inOpen,
                                                     SubArray<float>^ inHigh,
                                                     SubArray<float>^ inLow,
                                                     SubArray<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return Cdl3StarsInSouth( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<double>(inOpen,0),
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return Cdl3StarsInSouth( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<float>(inOpen,0),
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         static enum class RetCode Cdl3StarsInSouth( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3STARSINSOUTH Core::Cdl3StarsInSouth
         #define TA_CDL3STARSINSOUTH_Lookback Core::Cdl3StarsInSouthLookback

         static int Cdl3WhiteSoldiersLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return Cdl3WhiteSoldiers( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return Cdl3WhiteSoldiers( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode Cdl3WhiteSoldiers( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDL3WHITESOLDIERS Core::Cdl3WhiteSoldiers
         #define TA_CDL3WHITESOLDIERS_Lookback Core::Cdl3WhiteSoldiersLookback

         static int CdlAbandonedBabyLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inOpen,
                                                     SubArray<double>^ inHigh,
                                                     SubArray<double>^ inLow,
                                                     SubArray<double>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inOpen,
                                                     SubArray<float>^ inHigh,
                                                     SubArray<float>^ inLow,
                                                     SubArray<float>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlAbandonedBaby( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<double>(inOpen,0),
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                         optInPenetration, /* From 0 to TA_REAL_MAX */
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlAbandonedBaby( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<float>(inOpen,0),
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                         optInPenetration, /* From 0 to TA_REAL_MAX */
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         static enum class RetCode CdlAbandonedBaby( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         #endif

         #define TA_CDLABANDONEDBABY Core::CdlAbandonedBaby
         #define TA_CDLABANDONEDBABY_Lookback Core::CdlAbandonedBabyLookback

         static int CdlAdvanceBlockLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<double>^ inOpen,
                                                    SubArray<double>^ inHigh,
                                                    SubArray<double>^ inLow,
                                                    SubArray<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<float>^ inOpen,
                                                    SubArray<float>^ inHigh,
                                                    SubArray<float>^ inLow,
                                                    SubArray<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlAdvanceBlock( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<double>(inOpen,0),
                        gcnew SubArrayFrom1D<double>(inHigh,0),
                        gcnew SubArrayFrom1D<double>(inLow,0),
                        gcnew SubArrayFrom1D<double>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlAdvanceBlock( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<float>(inOpen,0),
                        gcnew SubArrayFrom1D<float>(inHigh,0),
                        gcnew SubArrayFrom1D<float>(inLow,0),
                        gcnew SubArrayFrom1D<float>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         static enum class RetCode CdlAdvanceBlock( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         #endif

         #define TA_CDLADVANCEBLOCK Core::CdlAdvanceBlock
         #define TA_CDLADVANCEBLOCK_Lookback Core::CdlAdvanceBlockLookback

         static int CdlBeltHoldLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlBeltHold( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlBeltHold( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlBeltHold( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLBELTHOLD Core::CdlBeltHold
         #define TA_CDLBELTHOLD_Lookback Core::CdlBeltHoldLookback

         static int CdlBreakawayLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inOpen,
                                                 SubArray<double>^ inHigh,
                                                 SubArray<double>^ inLow,
                                                 SubArray<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inOpen,
                                                 SubArray<float>^ inHigh,
                                                 SubArray<float>^ inLow,
                                                 SubArray<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlBreakaway( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<double>(inOpen,0),
                     gcnew SubArrayFrom1D<double>(inHigh,0),
                     gcnew SubArrayFrom1D<double>(inLow,0),
                     gcnew SubArrayFrom1D<double>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlBreakaway( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<float>(inOpen,0),
                     gcnew SubArrayFrom1D<float>(inHigh,0),
                     gcnew SubArrayFrom1D<float>(inLow,0),
                     gcnew SubArrayFrom1D<float>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         static enum class RetCode CdlBreakaway( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         #endif

         #define TA_CDLBREAKAWAY Core::CdlBreakaway
         #define TA_CDLBREAKAWAY_Lookback Core::CdlBreakawayLookback

         static int CdlClosingMarubozuLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlClosingMarubozu( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlClosingMarubozu( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlClosingMarubozu( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLCLOSINGMARUBOZU Core::CdlClosingMarubozu
         #define TA_CDLCLOSINGMARUBOZU_Lookback Core::CdlClosingMarubozuLookback

         static int CdlConcealBabysWallLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<double>^ inOpen,
                                                        SubArray<double>^ inHigh,
                                                        SubArray<double>^ inLow,
                                                        SubArray<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<float>^ inOpen,
                                                        SubArray<float>^ inHigh,
                                                        SubArray<float>^ inLow,
                                                        SubArray<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlConcealBabysWall( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<double>(inOpen,0),
                            gcnew SubArrayFrom1D<double>(inHigh,0),
                            gcnew SubArrayFrom1D<double>(inLow,0),
                            gcnew SubArrayFrom1D<double>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlConcealBabysWall( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<float>(inOpen,0),
                            gcnew SubArrayFrom1D<float>(inHigh,0),
                            gcnew SubArrayFrom1D<float>(inLow,0),
                            gcnew SubArrayFrom1D<float>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         static enum class RetCode CdlConcealBabysWall( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         #endif

         #define TA_CDLCONCEALBABYSWALL Core::CdlConcealBabysWall
         #define TA_CDLCONCEALBABYSWALL_Lookback Core::CdlConcealBabysWallLookback

         static int CdlCounterAttackLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inOpen,
                                                     SubArray<double>^ inHigh,
                                                     SubArray<double>^ inLow,
                                                     SubArray<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inOpen,
                                                     SubArray<float>^ inHigh,
                                                     SubArray<float>^ inLow,
                                                     SubArray<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlCounterAttack( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<double>(inOpen,0),
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlCounterAttack( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<float>(inOpen,0),
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         static enum class RetCode CdlCounterAttack( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         #endif

         #define TA_CDLCOUNTERATTACK Core::CdlCounterAttack
         #define TA_CDLCOUNTERATTACK_Lookback Core::CdlCounterAttackLookback

         static int CdlDarkCloudCoverLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlDarkCloudCover( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                          optInPenetration, /* From 0 to TA_REAL_MAX */
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlDarkCloudCover( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                          optInPenetration, /* From 0 to TA_REAL_MAX */
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode CdlDarkCloudCover( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDLDARKCLOUDCOVER Core::CdlDarkCloudCover
         #define TA_CDLDARKCLOUDCOVER_Lookback Core::CdlDarkCloudCoverLookback

         static int CdlDojiLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inOpen,
                                            SubArray<double>^ inHigh,
                                            SubArray<double>^ inLow,
                                            SubArray<double>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<int>^  outInteger );

         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inOpen,
                                            SubArray<float>^ inHigh,
                                            SubArray<float>^ inLow,
                                            SubArray<float>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<int>^  outInteger );

         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inOpen,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            cli::array<double>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<int>^  outInteger )
         { return CdlDoji( startIdx,   endIdx,
                gcnew SubArrayFrom1D<double>(inOpen,0),
                gcnew SubArrayFrom1D<double>(inHigh,0),
                gcnew SubArrayFrom1D<double>(inLow,0),
                gcnew SubArrayFrom1D<double>(inClose,0),
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inOpen,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            cli::array<float>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<int>^  outInteger )
         { return CdlDoji( startIdx,   endIdx,
                gcnew SubArrayFrom1D<float>(inOpen,0),
                gcnew SubArrayFrom1D<float>(inHigh,0),
                gcnew SubArrayFrom1D<float>(inLow,0),
                gcnew SubArrayFrom1D<float>(inClose,0),
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inOpen,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            cli::array<double>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<int>^  outInteger );
         static enum class RetCode CdlDoji( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inOpen,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            cli::array<float>^ inClose,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<int>^  outInteger );
         #endif

         #define TA_CDLDOJI Core::CdlDoji
         #define TA_CDLDOJI_Lookback Core::CdlDojiLookback

         static int CdlDojiStarLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlDojiStar( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlDojiStar( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlDojiStar( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLDOJISTAR Core::CdlDojiStar
         #define TA_CDLDOJISTAR_Lookback Core::CdlDojiStarLookback

         static int CdlDragonflyDojiLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inOpen,
                                                     SubArray<double>^ inHigh,
                                                     SubArray<double>^ inLow,
                                                     SubArray<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inOpen,
                                                     SubArray<float>^ inHigh,
                                                     SubArray<float>^ inLow,
                                                     SubArray<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlDragonflyDoji( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<double>(inOpen,0),
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlDragonflyDoji( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<float>(inOpen,0),
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         static enum class RetCode CdlDragonflyDoji( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         #endif

         #define TA_CDLDRAGONFLYDOJI Core::CdlDragonflyDoji
         #define TA_CDLDRAGONFLYDOJI_Lookback Core::CdlDragonflyDojiLookback

         static int CdlEngulfingLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inOpen,
                                                 SubArray<double>^ inHigh,
                                                 SubArray<double>^ inLow,
                                                 SubArray<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inOpen,
                                                 SubArray<float>^ inHigh,
                                                 SubArray<float>^ inLow,
                                                 SubArray<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlEngulfing( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<double>(inOpen,0),
                     gcnew SubArrayFrom1D<double>(inHigh,0),
                     gcnew SubArrayFrom1D<double>(inLow,0),
                     gcnew SubArrayFrom1D<double>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlEngulfing( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<float>(inOpen,0),
                     gcnew SubArrayFrom1D<float>(inHigh,0),
                     gcnew SubArrayFrom1D<float>(inLow,0),
                     gcnew SubArrayFrom1D<float>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         static enum class RetCode CdlEngulfing( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         #endif

         #define TA_CDLENGULFING Core::CdlEngulfing
         #define TA_CDLENGULFING_Lookback Core::CdlEngulfingLookback

         static int CdlEveningDojiStarLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlEveningDojiStar( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                           optInPenetration, /* From 0 to TA_REAL_MAX */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlEveningDojiStar( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                           optInPenetration, /* From 0 to TA_REAL_MAX */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlEveningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLEVENINGDOJISTAR Core::CdlEveningDojiStar
         #define TA_CDLEVENINGDOJISTAR_Lookback Core::CdlEveningDojiStarLookback

         static int CdlEveningStarLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlEveningStar( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                       optInPenetration, /* From 0 to TA_REAL_MAX */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlEveningStar( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                       optInPenetration, /* From 0 to TA_REAL_MAX */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlEveningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLEVENINGSTAR Core::CdlEveningStar
         #define TA_CDLEVENINGSTAR_Lookback Core::CdlEveningStarLookback

         static int CdlGapSideSideWhiteLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<double>^ inOpen,
                                                        SubArray<double>^ inHigh,
                                                        SubArray<double>^ inLow,
                                                        SubArray<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<float>^ inOpen,
                                                        SubArray<float>^ inHigh,
                                                        SubArray<float>^ inLow,
                                                        SubArray<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlGapSideSideWhite( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<double>(inOpen,0),
                            gcnew SubArrayFrom1D<double>(inHigh,0),
                            gcnew SubArrayFrom1D<double>(inLow,0),
                            gcnew SubArrayFrom1D<double>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlGapSideSideWhite( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<float>(inOpen,0),
                            gcnew SubArrayFrom1D<float>(inHigh,0),
                            gcnew SubArrayFrom1D<float>(inLow,0),
                            gcnew SubArrayFrom1D<float>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         static enum class RetCode CdlGapSideSideWhite( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         #endif

         #define TA_CDLGAPSIDESIDEWHITE Core::CdlGapSideSideWhite
         #define TA_CDLGAPSIDESIDEWHITE_Lookback Core::CdlGapSideSideWhiteLookback

         static int CdlGravestoneDojiLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlGravestoneDoji( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlGravestoneDoji( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode CdlGravestoneDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDLGRAVESTONEDOJI Core::CdlGravestoneDoji
         #define TA_CDLGRAVESTONEDOJI_Lookback Core::CdlGravestoneDojiLookback

         static int CdlHammerLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlHammer( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlHammer( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode CdlHammer( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHAMMER Core::CdlHammer
         #define TA_CDLHAMMER_Lookback Core::CdlHammerLookback

         static int CdlHangingManLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<double>^ inOpen,
                                                  SubArray<double>^ inHigh,
                                                  SubArray<double>^ inLow,
                                                  SubArray<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<int>^  outInteger );

         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<float>^ inOpen,
                                                  SubArray<float>^ inHigh,
                                                  SubArray<float>^ inLow,
                                                  SubArray<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<int>^  outInteger );

         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inOpen,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger )
         { return CdlHangingMan( startIdx,         endIdx,
                      gcnew SubArrayFrom1D<double>(inOpen,0),
                      gcnew SubArrayFrom1D<double>(inHigh,0),
                      gcnew SubArrayFrom1D<double>(inLow,0),
                      gcnew SubArrayFrom1D<double>(inClose,0),
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inOpen,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger )
         { return CdlHangingMan( startIdx,         endIdx,
                      gcnew SubArrayFrom1D<float>(inOpen,0),
                      gcnew SubArrayFrom1D<float>(inHigh,0),
                      gcnew SubArrayFrom1D<float>(inLow,0),
                      gcnew SubArrayFrom1D<float>(inClose,0),
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inOpen,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger );
         static enum class RetCode CdlHangingMan( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inOpen,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHANGINGMAN Core::CdlHangingMan
         #define TA_CDLHANGINGMAN_Lookback Core::CdlHangingManLookback

         static int CdlHaramiLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlHarami( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlHarami( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode CdlHarami( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHARAMI Core::CdlHarami
         #define TA_CDLHARAMI_Lookback Core::CdlHaramiLookback

         static int CdlHaramiCrossLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlHaramiCross( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlHaramiCross( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlHaramiCross( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHARAMICROSS Core::CdlHaramiCross
         #define TA_CDLHARAMICROSS_Lookback Core::CdlHaramiCrossLookback

         static int CdlHignWaveLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlHignWave( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlHignWave( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlHignWave( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHIGHWAVE Core::CdlHignWave
         #define TA_CDLHIGHWAVE_Lookback Core::CdlHignWaveLookback

         static int CdlHikkakeLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inOpen,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               SubArray<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inOpen,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               SubArray<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlHikkake( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<double>(inOpen,0),
                   gcnew SubArrayFrom1D<double>(inHigh,0),
                   gcnew SubArrayFrom1D<double>(inLow,0),
                   gcnew SubArrayFrom1D<double>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlHikkake( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<float>(inOpen,0),
                   gcnew SubArrayFrom1D<float>(inHigh,0),
                   gcnew SubArrayFrom1D<float>(inLow,0),
                   gcnew SubArrayFrom1D<float>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         static enum class RetCode CdlHikkake( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHIKKAKE Core::CdlHikkake
         #define TA_CDLHIKKAKE_Lookback Core::CdlHikkakeLookback

         static int CdlHikkakeModLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<double>^ inOpen,
                                                  SubArray<double>^ inHigh,
                                                  SubArray<double>^ inLow,
                                                  SubArray<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<int>^  outInteger );

         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<float>^ inOpen,
                                                  SubArray<float>^ inHigh,
                                                  SubArray<float>^ inLow,
                                                  SubArray<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<int>^  outInteger );

         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inOpen,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger )
         { return CdlHikkakeMod( startIdx,         endIdx,
                      gcnew SubArrayFrom1D<double>(inOpen,0),
                      gcnew SubArrayFrom1D<double>(inHigh,0),
                      gcnew SubArrayFrom1D<double>(inLow,0),
                      gcnew SubArrayFrom1D<double>(inClose,0),
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inOpen,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger )
         { return CdlHikkakeMod( startIdx,         endIdx,
                      gcnew SubArrayFrom1D<float>(inOpen,0),
                      gcnew SubArrayFrom1D<float>(inHigh,0),
                      gcnew SubArrayFrom1D<float>(inLow,0),
                      gcnew SubArrayFrom1D<float>(inClose,0),
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inOpen,
                                                  cli::array<double>^ inHigh,
                                                  cli::array<double>^ inLow,
                                                  cli::array<double>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger );
         static enum class RetCode CdlHikkakeMod( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inOpen,
                                                  cli::array<float>^ inHigh,
                                                  cli::array<float>^ inLow,
                                                  cli::array<float>^ inClose,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHIKKAKEMOD Core::CdlHikkakeMod
         #define TA_CDLHIKKAKEMOD_Lookback Core::CdlHikkakeModLookback

         static int CdlHomingPigeonLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<double>^ inOpen,
                                                    SubArray<double>^ inHigh,
                                                    SubArray<double>^ inLow,
                                                    SubArray<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<float>^ inOpen,
                                                    SubArray<float>^ inHigh,
                                                    SubArray<float>^ inLow,
                                                    SubArray<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlHomingPigeon( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<double>(inOpen,0),
                        gcnew SubArrayFrom1D<double>(inHigh,0),
                        gcnew SubArrayFrom1D<double>(inLow,0),
                        gcnew SubArrayFrom1D<double>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlHomingPigeon( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<float>(inOpen,0),
                        gcnew SubArrayFrom1D<float>(inHigh,0),
                        gcnew SubArrayFrom1D<float>(inLow,0),
                        gcnew SubArrayFrom1D<float>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         static enum class RetCode CdlHomingPigeon( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         #endif

         #define TA_CDLHOMINGPIGEON Core::CdlHomingPigeon
         #define TA_CDLHOMINGPIGEON_Lookback Core::CdlHomingPigeonLookback

         static int CdlIdentical3CrowsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlIdentical3Crows( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlIdentical3Crows( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlIdentical3Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLIDENTICAL3CROWS Core::CdlIdentical3Crows
         #define TA_CDLIDENTICAL3CROWS_Lookback Core::CdlIdentical3CrowsLookback

         static int CdlInNeckLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlInNeck( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlInNeck( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode CdlInNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDLINNECK Core::CdlInNeck
         #define TA_CDLINNECK_Lookback Core::CdlInNeckLookback

         static int CdlInvertedHammerLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlInvertedHammer( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlInvertedHammer( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode CdlInvertedHammer( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDLINVERTEDHAMMER Core::CdlInvertedHammer
         #define TA_CDLINVERTEDHAMMER_Lookback Core::CdlInvertedHammerLookback

         static int CdlKickingLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inOpen,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               SubArray<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inOpen,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               SubArray<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlKicking( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<double>(inOpen,0),
                   gcnew SubArrayFrom1D<double>(inHigh,0),
                   gcnew SubArrayFrom1D<double>(inLow,0),
                   gcnew SubArrayFrom1D<double>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlKicking( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<float>(inOpen,0),
                   gcnew SubArrayFrom1D<float>(inHigh,0),
                   gcnew SubArrayFrom1D<float>(inLow,0),
                   gcnew SubArrayFrom1D<float>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         static enum class RetCode CdlKicking( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         #endif

         #define TA_CDLKICKING Core::CdlKicking
         #define TA_CDLKICKING_Lookback Core::CdlKickingLookback

         static int CdlKickingByLengthLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlKickingByLength( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlKickingByLength( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlKickingByLength( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLKICKINGBYLENGTH Core::CdlKickingByLength
         #define TA_CDLKICKINGBYLENGTH_Lookback Core::CdlKickingByLengthLookback

         static int CdlLadderBottomLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<double>^ inOpen,
                                                    SubArray<double>^ inHigh,
                                                    SubArray<double>^ inLow,
                                                    SubArray<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<float>^ inOpen,
                                                    SubArray<float>^ inHigh,
                                                    SubArray<float>^ inLow,
                                                    SubArray<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlLadderBottom( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<double>(inOpen,0),
                        gcnew SubArrayFrom1D<double>(inHigh,0),
                        gcnew SubArrayFrom1D<double>(inLow,0),
                        gcnew SubArrayFrom1D<double>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlLadderBottom( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<float>(inOpen,0),
                        gcnew SubArrayFrom1D<float>(inHigh,0),
                        gcnew SubArrayFrom1D<float>(inLow,0),
                        gcnew SubArrayFrom1D<float>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         static enum class RetCode CdlLadderBottom( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         #endif

         #define TA_CDLLADDERBOTTOM Core::CdlLadderBottom
         #define TA_CDLLADDERBOTTOM_Lookback Core::CdlLadderBottomLookback

         static int CdlLongLeggedDojiLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlLongLeggedDoji( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlLongLeggedDoji( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode CdlLongLeggedDoji( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDLLONGLEGGEDDOJI Core::CdlLongLeggedDoji
         #define TA_CDLLONGLEGGEDDOJI_Lookback Core::CdlLongLeggedDojiLookback

         static int CdlLongLineLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlLongLine( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlLongLine( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlLongLine( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLLONGLINE Core::CdlLongLine
         #define TA_CDLLONGLINE_Lookback Core::CdlLongLineLookback

         static int CdlMarubozuLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlMarubozu( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlMarubozu( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlMarubozu( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLMARUBOZU Core::CdlMarubozu
         #define TA_CDLMARUBOZU_Lookback Core::CdlMarubozuLookback

         static int CdlMatchingLowLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlMatchingLow( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlMatchingLow( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlMatchingLow( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLMATCHINGLOW Core::CdlMatchingLow
         #define TA_CDLMATCHINGLOW_Lookback Core::CdlMatchingLowLookback

         static int CdlMatHoldLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inOpen,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               SubArray<double>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inOpen,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               SubArray<float>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlMatHold( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<double>(inOpen,0),
                   gcnew SubArrayFrom1D<double>(inHigh,0),
                   gcnew SubArrayFrom1D<double>(inLow,0),
                   gcnew SubArrayFrom1D<double>(inClose,0),
                   optInPenetration, /* From 0 to TA_REAL_MAX */
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlMatHold( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<float>(inOpen,0),
                   gcnew SubArrayFrom1D<float>(inHigh,0),
                   gcnew SubArrayFrom1D<float>(inLow,0),
                   gcnew SubArrayFrom1D<float>(inClose,0),
                   optInPenetration, /* From 0 to TA_REAL_MAX */
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         static enum class RetCode CdlMatHold( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         #endif

         #define TA_CDLMATHOLD Core::CdlMatHold
         #define TA_CDLMATHOLD_Lookback Core::CdlMatHoldLookback

         static int CdlMorningDojiStarLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlMorningDojiStar( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                           optInPenetration, /* From 0 to TA_REAL_MAX */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlMorningDojiStar( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                           optInPenetration, /* From 0 to TA_REAL_MAX */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlMorningDojiStar( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLMORNINGDOJISTAR Core::CdlMorningDojiStar
         #define TA_CDLMORNINGDOJISTAR_Lookback Core::CdlMorningDojiStarLookback

         static int CdlMorningStarLookback( double        optInPenetration );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlMorningStar( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                       optInPenetration, /* From 0 to TA_REAL_MAX */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlMorningStar( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                       optInPenetration, /* From 0 to TA_REAL_MAX */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlMorningStar( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   double        optInPenetration, /* From 0 to TA_REAL_MAX */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLMORNINGSTAR Core::CdlMorningStar
         #define TA_CDLMORNINGSTAR_Lookback Core::CdlMorningStarLookback

         static int CdlOnNeckLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlOnNeck( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlOnNeck( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode CdlOnNeck( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDLONNECK Core::CdlOnNeck
         #define TA_CDLONNECK_Lookback Core::CdlOnNeckLookback

         static int CdlPiercingLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inOpen,
                                                SubArray<double>^ inHigh,
                                                SubArray<double>^ inLow,
                                                SubArray<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inOpen,
                                                SubArray<float>^ inHigh,
                                                SubArray<float>^ inLow,
                                                SubArray<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlPiercing( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<double>(inOpen,0),
                    gcnew SubArrayFrom1D<double>(inHigh,0),
                    gcnew SubArrayFrom1D<double>(inLow,0),
                    gcnew SubArrayFrom1D<double>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return CdlPiercing( startIdx,       endIdx,
                    gcnew SubArrayFrom1D<float>(inOpen,0),
                    gcnew SubArrayFrom1D<float>(inHigh,0),
                    gcnew SubArrayFrom1D<float>(inLow,0),
                    gcnew SubArrayFrom1D<float>(inClose,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inOpen,
                                                cli::array<double>^ inHigh,
                                                cli::array<double>^ inLow,
                                                cli::array<double>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode CdlPiercing( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inOpen,
                                                cli::array<float>^ inHigh,
                                                cli::array<float>^ inLow,
                                                cli::array<float>^ inClose,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_CDLPIERCING Core::CdlPiercing
         #define TA_CDLPIERCING_Lookback Core::CdlPiercingLookback

         static int CdlRickshawManLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlRickshawMan( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlRickshawMan( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlRickshawMan( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLRICKSHAWMAN Core::CdlRickshawMan
         #define TA_CDLRICKSHAWMAN_Lookback Core::CdlRickshawManLookback

         static int CdlRiseFall3MethodsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<double>^ inOpen,
                                                        SubArray<double>^ inHigh,
                                                        SubArray<double>^ inLow,
                                                        SubArray<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<float>^ inOpen,
                                                        SubArray<float>^ inHigh,
                                                        SubArray<float>^ inLow,
                                                        SubArray<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlRiseFall3Methods( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<double>(inOpen,0),
                            gcnew SubArrayFrom1D<double>(inHigh,0),
                            gcnew SubArrayFrom1D<double>(inLow,0),
                            gcnew SubArrayFrom1D<double>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlRiseFall3Methods( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<float>(inOpen,0),
                            gcnew SubArrayFrom1D<float>(inHigh,0),
                            gcnew SubArrayFrom1D<float>(inLow,0),
                            gcnew SubArrayFrom1D<float>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         static enum class RetCode CdlRiseFall3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         #endif

         #define TA_CDLRISEFALL3METHODS Core::CdlRiseFall3Methods
         #define TA_CDLRISEFALL3METHODS_Lookback Core::CdlRiseFall3MethodsLookback

         static int CdlSeperatingLinesLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlSeperatingLines( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlSeperatingLines( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlSeperatingLines( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSEPARATINGLINES Core::CdlSeperatingLines
         #define TA_CDLSEPARATINGLINES_Lookback Core::CdlSeperatingLinesLookback

         static int CdlShootingStarLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<double>^ inOpen,
                                                    SubArray<double>^ inHigh,
                                                    SubArray<double>^ inLow,
                                                    SubArray<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<float>^ inOpen,
                                                    SubArray<float>^ inHigh,
                                                    SubArray<float>^ inLow,
                                                    SubArray<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlShootingStar( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<double>(inOpen,0),
                        gcnew SubArrayFrom1D<double>(inHigh,0),
                        gcnew SubArrayFrom1D<double>(inLow,0),
                        gcnew SubArrayFrom1D<double>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlShootingStar( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<float>(inOpen,0),
                        gcnew SubArrayFrom1D<float>(inHigh,0),
                        gcnew SubArrayFrom1D<float>(inLow,0),
                        gcnew SubArrayFrom1D<float>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         static enum class RetCode CdlShootingStar( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSHOOTINGSTAR Core::CdlShootingStar
         #define TA_CDLSHOOTINGSTAR_Lookback Core::CdlShootingStarLookback

         static int CdlShortLineLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inOpen,
                                                 SubArray<double>^ inHigh,
                                                 SubArray<double>^ inLow,
                                                 SubArray<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inOpen,
                                                 SubArray<float>^ inHigh,
                                                 SubArray<float>^ inLow,
                                                 SubArray<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlShortLine( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<double>(inOpen,0),
                     gcnew SubArrayFrom1D<double>(inHigh,0),
                     gcnew SubArrayFrom1D<double>(inLow,0),
                     gcnew SubArrayFrom1D<double>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlShortLine( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<float>(inOpen,0),
                     gcnew SubArrayFrom1D<float>(inHigh,0),
                     gcnew SubArrayFrom1D<float>(inLow,0),
                     gcnew SubArrayFrom1D<float>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         static enum class RetCode CdlShortLine( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSHORTLINE Core::CdlShortLine
         #define TA_CDLSHORTLINE_Lookback Core::CdlShortLineLookback

         static int CdlSpinningTopLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inOpen,
                                                   SubArray<double>^ inHigh,
                                                   SubArray<double>^ inLow,
                                                   SubArray<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inOpen,
                                                   SubArray<float>^ inHigh,
                                                   SubArray<float>^ inLow,
                                                   SubArray<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<int>^  outInteger );

         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlSpinningTop( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<double>(inOpen,0),
                       gcnew SubArrayFrom1D<double>(inHigh,0),
                       gcnew SubArrayFrom1D<double>(inLow,0),
                       gcnew SubArrayFrom1D<double>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger )
         { return CdlSpinningTop( startIdx,          endIdx,
                       gcnew SubArrayFrom1D<float>(inOpen,0),
                       gcnew SubArrayFrom1D<float>(inHigh,0),
                       gcnew SubArrayFrom1D<float>(inLow,0),
                       gcnew SubArrayFrom1D<float>(inClose,0),
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inOpen,
                                                   cli::array<double>^ inHigh,
                                                   cli::array<double>^ inLow,
                                                   cli::array<double>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         static enum class RetCode CdlSpinningTop( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inOpen,
                                                   cli::array<float>^ inHigh,
                                                   cli::array<float>^ inLow,
                                                   cli::array<float>^ inClose,
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSPINNINGTOP Core::CdlSpinningTop
         #define TA_CDLSPINNINGTOP_Lookback Core::CdlSpinningTopLookback

         static int CdlStalledPatternLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<double>^ inOpen,
                                                      SubArray<double>^ inHigh,
                                                      SubArray<double>^ inLow,
                                                      SubArray<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      SubArray<float>^ inOpen,
                                                      SubArray<float>^ inHigh,
                                                      SubArray<float>^ inLow,
                                                      SubArray<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      SubArray<int>^  outInteger );

         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlStalledPattern( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<double>(inOpen,0),
                          gcnew SubArrayFrom1D<double>(inHigh,0),
                          gcnew SubArrayFrom1D<double>(inLow,0),
                          gcnew SubArrayFrom1D<double>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger )
         { return CdlStalledPattern( startIdx,             endIdx,
                          gcnew SubArrayFrom1D<float>(inOpen,0),
                          gcnew SubArrayFrom1D<float>(inHigh,0),
                          gcnew SubArrayFrom1D<float>(inLow,0),
                          gcnew SubArrayFrom1D<float>(inClose,0),
                         outBegIdx,
                         outNBElement,
                            gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<double>^ inOpen,
                                                      cli::array<double>^ inHigh,
                                                      cli::array<double>^ inLow,
                                                      cli::array<double>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         static enum class RetCode CdlStalledPattern( int    startIdx,
                                                      int    endIdx,
                                                      cli::array<float>^ inOpen,
                                                      cli::array<float>^ inHigh,
                                                      cli::array<float>^ inLow,
                                                      cli::array<float>^ inClose,
                                                      [Out]int%    outBegIdx,
                                                      [Out]int%    outNBElement,
                                                      cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSTALLEDPATTERN Core::CdlStalledPattern
         #define TA_CDLSTALLEDPATTERN_Lookback Core::CdlStalledPatternLookback

         static int CdlStickSandwichLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<double>^ inOpen,
                                                     SubArray<double>^ inHigh,
                                                     SubArray<double>^ inLow,
                                                     SubArray<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     SubArray<float>^ inOpen,
                                                     SubArray<float>^ inHigh,
                                                     SubArray<float>^ inLow,
                                                     SubArray<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     SubArray<int>^  outInteger );

         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlStickSandwich( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<double>(inOpen,0),
                         gcnew SubArrayFrom1D<double>(inHigh,0),
                         gcnew SubArrayFrom1D<double>(inLow,0),
                         gcnew SubArrayFrom1D<double>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger )
         { return CdlStickSandwich( startIdx,            endIdx,
                         gcnew SubArrayFrom1D<float>(inOpen,0),
                         gcnew SubArrayFrom1D<float>(inHigh,0),
                         gcnew SubArrayFrom1D<float>(inLow,0),
                         gcnew SubArrayFrom1D<float>(inClose,0),
                        outBegIdx,
                        outNBElement,
                           gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<double>^ inOpen,
                                                     cli::array<double>^ inHigh,
                                                     cli::array<double>^ inLow,
                                                     cli::array<double>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         static enum class RetCode CdlStickSandwich( int    startIdx,
                                                     int    endIdx,
                                                     cli::array<float>^ inOpen,
                                                     cli::array<float>^ inHigh,
                                                     cli::array<float>^ inLow,
                                                     cli::array<float>^ inClose,
                                                     [Out]int%    outBegIdx,
                                                     [Out]int%    outNBElement,
                                                     cli::array<int>^  outInteger );
         #endif

         #define TA_CDLSTICKSANDWICH Core::CdlStickSandwich
         #define TA_CDLSTICKSANDWICH_Lookback Core::CdlStickSandwichLookback

         static int CdlTakuriLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inOpen,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inOpen,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<int>^  outInteger );

         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlTakuri( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inOpen,0),
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger )
         { return CdlTakuri( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inOpen,0),
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inOpen,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         static enum class RetCode CdlTakuri( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inOpen,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<int>^  outInteger );
         #endif

         #define TA_CDLTAKURI Core::CdlTakuri
         #define TA_CDLTAKURI_Lookback Core::CdlTakuriLookback

         static int CdlTasukiGapLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inOpen,
                                                 SubArray<double>^ inHigh,
                                                 SubArray<double>^ inLow,
                                                 SubArray<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inOpen,
                                                 SubArray<float>^ inHigh,
                                                 SubArray<float>^ inLow,
                                                 SubArray<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlTasukiGap( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<double>(inOpen,0),
                     gcnew SubArrayFrom1D<double>(inHigh,0),
                     gcnew SubArrayFrom1D<double>(inLow,0),
                     gcnew SubArrayFrom1D<double>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlTasukiGap( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<float>(inOpen,0),
                     gcnew SubArrayFrom1D<float>(inHigh,0),
                     gcnew SubArrayFrom1D<float>(inLow,0),
                     gcnew SubArrayFrom1D<float>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         static enum class RetCode CdlTasukiGap( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         #endif

         #define TA_CDLTASUKIGAP Core::CdlTasukiGap
         #define TA_CDLTASUKIGAP_Lookback Core::CdlTasukiGapLookback

         static int CdlThrustingLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<double>^ inOpen,
                                                 SubArray<double>^ inHigh,
                                                 SubArray<double>^ inLow,
                                                 SubArray<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 SubArray<float>^ inOpen,
                                                 SubArray<float>^ inHigh,
                                                 SubArray<float>^ inLow,
                                                 SubArray<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 SubArray<int>^  outInteger );

         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlThrusting( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<double>(inOpen,0),
                     gcnew SubArrayFrom1D<double>(inHigh,0),
                     gcnew SubArrayFrom1D<double>(inLow,0),
                     gcnew SubArrayFrom1D<double>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger )
         { return CdlThrusting( startIdx,        endIdx,
                     gcnew SubArrayFrom1D<float>(inOpen,0),
                     gcnew SubArrayFrom1D<float>(inHigh,0),
                     gcnew SubArrayFrom1D<float>(inLow,0),
                     gcnew SubArrayFrom1D<float>(inClose,0),
                    outBegIdx,
                    outNBElement,
                       gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<double>^ inOpen,
                                                 cli::array<double>^ inHigh,
                                                 cli::array<double>^ inLow,
                                                 cli::array<double>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         static enum class RetCode CdlThrusting( int    startIdx,
                                                 int    endIdx,
                                                 cli::array<float>^ inOpen,
                                                 cli::array<float>^ inHigh,
                                                 cli::array<float>^ inLow,
                                                 cli::array<float>^ inClose,
                                                 [Out]int%    outBegIdx,
                                                 [Out]int%    outNBElement,
                                                 cli::array<int>^  outInteger );
         #endif

         #define TA_CDLTHRUSTING Core::CdlThrusting
         #define TA_CDLTHRUSTING_Lookback Core::CdlThrustingLookback

         static int CdlTristarLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inOpen,
                                               SubArray<double>^ inHigh,
                                               SubArray<double>^ inLow,
                                               SubArray<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inOpen,
                                               SubArray<float>^ inHigh,
                                               SubArray<float>^ inLow,
                                               SubArray<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<int>^  outInteger );

         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlTristar( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<double>(inOpen,0),
                   gcnew SubArrayFrom1D<double>(inHigh,0),
                   gcnew SubArrayFrom1D<double>(inLow,0),
                   gcnew SubArrayFrom1D<double>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger )
         { return CdlTristar( startIdx,      endIdx,
                   gcnew SubArrayFrom1D<float>(inOpen,0),
                   gcnew SubArrayFrom1D<float>(inHigh,0),
                   gcnew SubArrayFrom1D<float>(inLow,0),
                   gcnew SubArrayFrom1D<float>(inClose,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inOpen,
                                               cli::array<double>^ inHigh,
                                               cli::array<double>^ inLow,
                                               cli::array<double>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         static enum class RetCode CdlTristar( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inOpen,
                                               cli::array<float>^ inHigh,
                                               cli::array<float>^ inLow,
                                               cli::array<float>^ inClose,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<int>^  outInteger );
         #endif

         #define TA_CDLTRISTAR Core::CdlTristar
         #define TA_CDLTRISTAR_Lookback Core::CdlTristarLookback

         static int CdlUnique3RiverLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<double>^ inOpen,
                                                    SubArray<double>^ inHigh,
                                                    SubArray<double>^ inLow,
                                                    SubArray<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    SubArray<float>^ inOpen,
                                                    SubArray<float>^ inHigh,
                                                    SubArray<float>^ inLow,
                                                    SubArray<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    SubArray<int>^  outInteger );

         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlUnique3River( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<double>(inOpen,0),
                        gcnew SubArrayFrom1D<double>(inHigh,0),
                        gcnew SubArrayFrom1D<double>(inLow,0),
                        gcnew SubArrayFrom1D<double>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger )
         { return CdlUnique3River( startIdx,           endIdx,
                        gcnew SubArrayFrom1D<float>(inOpen,0),
                        gcnew SubArrayFrom1D<float>(inHigh,0),
                        gcnew SubArrayFrom1D<float>(inLow,0),
                        gcnew SubArrayFrom1D<float>(inClose,0),
                       outBegIdx,
                       outNBElement,
                          gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<double>^ inOpen,
                                                    cli::array<double>^ inHigh,
                                                    cli::array<double>^ inLow,
                                                    cli::array<double>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         static enum class RetCode CdlUnique3River( int    startIdx,
                                                    int    endIdx,
                                                    cli::array<float>^ inOpen,
                                                    cli::array<float>^ inHigh,
                                                    cli::array<float>^ inLow,
                                                    cli::array<float>^ inClose,
                                                    [Out]int%    outBegIdx,
                                                    [Out]int%    outNBElement,
                                                    cli::array<int>^  outInteger );
         #endif

         #define TA_CDLUNIQUE3RIVER Core::CdlUnique3River
         #define TA_CDLUNIQUE3RIVER_Lookback Core::CdlUnique3RiverLookback

         static int CdlUpsideGap2CrowsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inOpen,
                                                       SubArray<double>^ inHigh,
                                                       SubArray<double>^ inLow,
                                                       SubArray<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inOpen,
                                                       SubArray<float>^ inHigh,
                                                       SubArray<float>^ inLow,
                                                       SubArray<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<int>^  outInteger );

         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlUpsideGap2Crows( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<double>(inOpen,0),
                           gcnew SubArrayFrom1D<double>(inHigh,0),
                           gcnew SubArrayFrom1D<double>(inLow,0),
                           gcnew SubArrayFrom1D<double>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger )
         { return CdlUpsideGap2Crows( startIdx,              endIdx,
                           gcnew SubArrayFrom1D<float>(inOpen,0),
                           gcnew SubArrayFrom1D<float>(inHigh,0),
                           gcnew SubArrayFrom1D<float>(inLow,0),
                           gcnew SubArrayFrom1D<float>(inClose,0),
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inOpen,
                                                       cli::array<double>^ inHigh,
                                                       cli::array<double>^ inLow,
                                                       cli::array<double>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         static enum class RetCode CdlUpsideGap2Crows( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inOpen,
                                                       cli::array<float>^ inHigh,
                                                       cli::array<float>^ inLow,
                                                       cli::array<float>^ inClose,
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<int>^  outInteger );
         #endif

         #define TA_CDLUPSIDEGAP2CROWS Core::CdlUpsideGap2Crows
         #define TA_CDLUPSIDEGAP2CROWS_Lookback Core::CdlUpsideGap2CrowsLookback

         static int CdlXSideGap3MethodsLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<double>^ inOpen,
                                                        SubArray<double>^ inHigh,
                                                        SubArray<double>^ inLow,
                                                        SubArray<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        SubArray<float>^ inOpen,
                                                        SubArray<float>^ inHigh,
                                                        SubArray<float>^ inLow,
                                                        SubArray<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        SubArray<int>^  outInteger );

         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlXSideGap3Methods( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<double>(inOpen,0),
                            gcnew SubArrayFrom1D<double>(inHigh,0),
                            gcnew SubArrayFrom1D<double>(inLow,0),
                            gcnew SubArrayFrom1D<double>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger )
         { return CdlXSideGap3Methods( startIdx,               endIdx,
                            gcnew SubArrayFrom1D<float>(inOpen,0),
                            gcnew SubArrayFrom1D<float>(inHigh,0),
                            gcnew SubArrayFrom1D<float>(inLow,0),
                            gcnew SubArrayFrom1D<float>(inClose,0),
                           outBegIdx,
                           outNBElement,
                              gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<double>^ inOpen,
                                                        cli::array<double>^ inHigh,
                                                        cli::array<double>^ inLow,
                                                        cli::array<double>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         static enum class RetCode CdlXSideGap3Methods( int    startIdx,
                                                        int    endIdx,
                                                        cli::array<float>^ inOpen,
                                                        cli::array<float>^ inHigh,
                                                        cli::array<float>^ inLow,
                                                        cli::array<float>^ inClose,
                                                        [Out]int%    outBegIdx,
                                                        [Out]int%    outNBElement,
                                                        cli::array<int>^  outInteger );
         #endif

         #define TA_CDLXSIDEGAP3METHODS Core::CdlXSideGap3Methods
         #define TA_CDLXSIDEGAP3METHODS_Lookback Core::CdlXSideGap3MethodsLookback

         static int CeilLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Ceil( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Ceil( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Ceil( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_CEIL Core::Ceil
         #define TA_CEIL_Lookback Core::CeilLookback

         static int CmoLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cmo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cmo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Cmo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_CMO Core::Cmo
         #define TA_CMO_Lookback Core::CmoLookback

         static int CorrelLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal0,
                                           SubArray<double>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal0,
                                           SubArray<float>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal0,
                                           cli::array<double>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return Correl( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal0,0),
                           gcnew SubArrayFrom1D<double>(inReal1,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal0,
                                           cli::array<float>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return Correl( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal0,0),
                           gcnew SubArrayFrom1D<float>(inReal1,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal0,
                                           cli::array<double>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode Correl( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal0,
                                           cli::array<float>^ inReal1,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_CORREL Core::Correl
         #define TA_CORREL_Lookback Core::CorrelLookback

         static int CosLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cos( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Cos( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Cos( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_COS Core::Cos
         #define TA_COS_Lookback Core::CosLookback

         static int CoshLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Cosh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Cosh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Cosh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_COSH Core::Cosh
         #define TA_COSH_Lookback Core::CoshLookback

         static int DemaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Dema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Dema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Dema( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_DEMA Core::Dema
         #define TA_DEMA_Lookback Core::DemaLookback

         static int DivLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal0,
                                        SubArray<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal0,
                                        SubArray<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Div( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal0,0),
                          gcnew SubArrayFrom1D<double>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Div( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal0,0),
                          gcnew SubArrayFrom1D<float>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Div( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_DIV Core::Div
         #define TA_DIV_Lookback Core::DivLookback

         static int DxLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       SubArray<double>^ inHigh,
                                       SubArray<double>^ inLow,
                                       SubArray<double>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       SubArray<float>^ inHigh,
                                       SubArray<float>^ inLow,
                                       SubArray<float>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inHigh,
                                       cli::array<double>^ inLow,
                                       cli::array<double>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Dx( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inHigh,
                                       cli::array<float>^ inLow,
                                       cli::array<float>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Dx( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inHigh,
                                       cli::array<double>^ inLow,
                                       cli::array<double>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         static enum class RetCode Dx( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inHigh,
                                       cli::array<float>^ inLow,
                                       cli::array<float>^ inClose,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         #endif

         #define TA_DX Core::Dx
         #define TA_DX_Lookback Core::DxLookback

         static int EmaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Ema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Ema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Ema( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_EMA Core::Ema
         #define TA_EMA_Lookback Core::EmaLookback

         static int ExpLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Exp( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Exp( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Exp( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_EXP Core::Exp
         #define TA_EXP_Lookback Core::ExpLookback

         static int FloorLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Floor( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Floor( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         static enum class RetCode Floor( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         #endif

         #define TA_FLOOR Core::Floor
         #define TA_FLOOR_Lookback Core::FloorLookback

         static int HtDcPeriodLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               SubArray<double>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<double>^  outReal );

         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               SubArray<float>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               SubArray<double>^  outReal );

         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outReal )
         { return HtDcPeriod( startIdx,      endIdx,
                               gcnew SubArrayFrom1D<double>(inReal,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outReal )
         { return HtDcPeriod( startIdx,      endIdx,
                               gcnew SubArrayFrom1D<float>(inReal,0),
                  outBegIdx,
                  outNBElement,
                     gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               cli::array<double>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outReal );
         static enum class RetCode HtDcPeriod( int    startIdx,
                                               int    endIdx,
                                               cli::array<float>^ inReal,
                                               [Out]int%    outBegIdx,
                                               [Out]int%    outNBElement,
                                               cli::array<double>^  outReal );
         #endif

         #define TA_HT_DCPERIOD Core::HtDcPeriod
         #define TA_HT_DCPERIOD_Lookback Core::HtDcPeriodLookback

         static int HtDcPhaseLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return HtDcPhase( startIdx,     endIdx,
                              gcnew SubArrayFrom1D<double>(inReal,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return HtDcPhase( startIdx,     endIdx,
                              gcnew SubArrayFrom1D<float>(inReal,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         static enum class RetCode HtDcPhase( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         #endif

         #define TA_HT_DCPHASE Core::HtDcPhase
         #define TA_HT_DCPHASE_Lookback Core::HtDcPhaseLookback

         static int HtPhasorLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outInPhase,
                                             SubArray<double>^  outQuadrature );

         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outInPhase,
                                             SubArray<double>^  outQuadrature );

         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outInPhase,
                                             cli::array<double>^  outQuadrature )
         { return HtPhasor( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outInPhase,0),
                   gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outInPhase,
                                             cli::array<double>^  outQuadrature )
         { return HtPhasor( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outInPhase,0),
                   gcnew SubArrayFrom1D<double>(outQuadrature,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outInPhase,
                                             cli::array<double>^  outQuadrature );
         static enum class RetCode HtPhasor( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outInPhase,
                                             cli::array<double>^  outQuadrature );
         #endif

         #define TA_HT_PHASOR Core::HtPhasor
         #define TA_HT_PHASOR_Lookback Core::HtPhasorLookback

         static int HtSineLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outSine,
                                           SubArray<double>^  outLeadSine );

         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outSine,
                                           SubArray<double>^  outLeadSine );

         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outSine,
                                           cli::array<double>^  outLeadSine )
         { return HtSine( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal,0),
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outSine,0),
                 gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outSine,
                                           cli::array<double>^  outLeadSine )
         { return HtSine( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal,0),
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outSine,0),
                 gcnew SubArrayFrom1D<double>(outLeadSine,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outSine,
                                           cli::array<double>^  outLeadSine );
         static enum class RetCode HtSine( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outSine,
                                           cli::array<double>^  outLeadSine );
         #endif

         #define TA_HT_SINE Core::HtSine
         #define TA_HT_SINE_Lookback Core::HtSineLookback

         static int HtTrendlineLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outReal );

         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<double>^  outReal );

         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outReal )
         { return HtTrendline( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<double>(inReal,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outReal )
         { return HtTrendline( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<float>(inReal,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outReal );
         static enum class RetCode HtTrendline( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<double>^  outReal );
         #endif

         #define TA_HT_TRENDLINE Core::HtTrendline
         #define TA_HT_TRENDLINE_Lookback Core::HtTrendlineLookback

         static int HtTrendModeLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outInteger );

         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return HtTrendMode( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<double>(inReal,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger )
         { return HtTrendMode( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<float>(inReal,0),
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         static enum class RetCode HtTrendMode( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outInteger );
         #endif

         #define TA_HT_TRENDMODE Core::HtTrendMode
         #define TA_HT_TRENDMODE_Lookback Core::HtTrendModeLookback

         static int ImiLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inOpen,
                                        SubArray<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inOpen,
                                        SubArray<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inOpen,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Imi( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inOpen,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inOpen,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Imi( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inOpen,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inOpen,
                                        cli::array<double>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Imi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inOpen,
                                        cli::array<float>^ inClose,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_IMI Core::Imi
         #define TA_IMI_Lookback Core::ImiLookback

         static int KamaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Kama( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Kama( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Kama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_KAMA Core::Kama
         #define TA_KAMA_Lookback Core::KamaLookback

         static int LinearRegLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return LinearReg( startIdx,     endIdx,
                              gcnew SubArrayFrom1D<double>(inReal,0),
                  optInTimePeriod, /* From 2 to 100000 */
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return LinearReg( startIdx,     endIdx,
                              gcnew SubArrayFrom1D<float>(inReal,0),
                  optInTimePeriod, /* From 2 to 100000 */
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         static enum class RetCode LinearReg( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inReal,
                                              int           optInTimePeriod, /* From 2 to 100000 */
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         #endif

         #define TA_LINEARREG Core::LinearReg
         #define TA_LINEARREG_Lookback Core::LinearRegLookback

         static int LinearRegAngleLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outReal );

         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outReal );

         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal )
         { return LinearRegAngle( startIdx,          endIdx,
                                   gcnew SubArrayFrom1D<double>(inReal,0),
                       optInTimePeriod, /* From 2 to 100000 */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal )
         { return LinearRegAngle( startIdx,          endIdx,
                                   gcnew SubArrayFrom1D<float>(inReal,0),
                       optInTimePeriod, /* From 2 to 100000 */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal );
         static enum class RetCode LinearRegAngle( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal );
         #endif

         #define TA_LINEARREG_ANGLE Core::LinearRegAngle
         #define TA_LINEARREG_ANGLE_Lookback Core::LinearRegAngleLookback

         static int LinearRegInterceptLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<double>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<double>^  outReal );

         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       SubArray<float>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       SubArray<double>^  outReal );

         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<double>^  outReal )
         { return LinearRegIntercept( startIdx,              endIdx,
                                       gcnew SubArrayFrom1D<double>(inReal,0),
                           optInTimePeriod, /* From 2 to 100000 */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<double>^  outReal )
         { return LinearRegIntercept( startIdx,              endIdx,
                                       gcnew SubArrayFrom1D<float>(inReal,0),
                           optInTimePeriod, /* From 2 to 100000 */
                          outBegIdx,
                          outNBElement,
                             gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<double>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<double>^  outReal );
         static enum class RetCode LinearRegIntercept( int    startIdx,
                                                       int    endIdx,
                                                       cli::array<float>^ inReal,
                                                       int           optInTimePeriod, /* From 2 to 100000 */
                                                       [Out]int%    outBegIdx,
                                                       [Out]int%    outNBElement,
                                                       cli::array<double>^  outReal );
         #endif

         #define TA_LINEARREG_INTERCEPT Core::LinearRegIntercept
         #define TA_LINEARREG_INTERCEPT_Lookback Core::LinearRegInterceptLookback

         static int LinearRegSlopeLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outReal );

         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   SubArray<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   SubArray<double>^  outReal );

         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal )
         { return LinearRegSlope( startIdx,          endIdx,
                                   gcnew SubArrayFrom1D<double>(inReal,0),
                       optInTimePeriod, /* From 2 to 100000 */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal )
         { return LinearRegSlope( startIdx,          endIdx,
                                   gcnew SubArrayFrom1D<float>(inReal,0),
                       optInTimePeriod, /* From 2 to 100000 */
                      outBegIdx,
                      outNBElement,
                         gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<double>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal );
         static enum class RetCode LinearRegSlope( int    startIdx,
                                                   int    endIdx,
                                                   cli::array<float>^ inReal,
                                                   int           optInTimePeriod, /* From 2 to 100000 */
                                                   [Out]int%    outBegIdx,
                                                   [Out]int%    outNBElement,
                                                   cli::array<double>^  outReal );
         #endif

         #define TA_LINEARREG_SLOPE Core::LinearRegSlope
         #define TA_LINEARREG_SLOPE_Lookback Core::LinearRegSlopeLookback

         static int LnLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       SubArray<double>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       SubArray<float>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Ln( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return Ln( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         static enum class RetCode Ln( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inReal,
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         #endif

         #define TA_LN Core::Ln
         #define TA_LN_Lookback Core::LnLookback

         static int Log10Lookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Log10( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Log10( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         static enum class RetCode Log10( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         #endif

         #define TA_LOG10 Core::Log10
         #define TA_LOG10_Lookback Core::Log10Lookback

         static int MovingAverageLookback( int           optInTimePeriod, /* From 1 to 100000 */
                                         MAType        optInMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<double>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outReal );

         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  SubArray<float>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  SubArray<double>^  outReal );

         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outReal )
         { return MovingAverage( startIdx,         endIdx,
                                  gcnew SubArrayFrom1D<double>(inReal,0),
                      optInTimePeriod, /* From 1 to 100000 */
                      optInMAType,
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outReal )
         { return MovingAverage( startIdx,         endIdx,
                                  gcnew SubArrayFrom1D<float>(inReal,0),
                      optInTimePeriod, /* From 1 to 100000 */
                      optInMAType,
                     outBegIdx,
                     outNBElement,
                        gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<double>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outReal );
         static enum class RetCode MovingAverage( int    startIdx,
                                                  int    endIdx,
                                                  cli::array<float>^ inReal,
                                                  int           optInTimePeriod, /* From 1 to 100000 */
                                                  MAType        optInMAType,
                                                  [Out]int%    outBegIdx,
                                                  [Out]int%    outNBElement,
                                                  cli::array<double>^  outReal );
         #endif

         #define TA_MA Core::MovingAverage
         #define TA_MA_Lookback Core::MovingAverageLookback

         static int MacdLookback( int           optInFastPeriod, /* From 2 to 100000 */
                                int           optInSlowPeriod, /* From 2 to 100000 */
                                int           optInSignalPeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMACD,
                                         SubArray<double>^  outMACDSignal,
                                         SubArray<double>^  outMACDHist );

         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMACD,
                                         SubArray<double>^  outMACDSignal,
                                         SubArray<double>^  outMACDHist );

         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist )
         { return Macd( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInSignalPeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outMACD,0),
                gcnew SubArrayFrom1D<double>(outMACDSignal,0),
                gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist )
         { return Macd( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInSignalPeriod, /* From 1 to 100000 */
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
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist );
         static enum class RetCode Macd( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInFastPeriod, /* From 2 to 100000 */
                                         int           optInSlowPeriod, /* From 2 to 100000 */
                                         int           optInSignalPeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMACD,
                                         cli::array<double>^  outMACDSignal,
                                         cli::array<double>^  outMACDHist );
         #endif

         #define TA_MACD Core::Macd
         #define TA_MACD_Lookback Core::MacdLookback

         static int MacdExtLookback( int           optInFastPeriod, /* From 2 to 100000 */
                                   MAType        optInFastMAType,
                                   int           optInSlowPeriod, /* From 2 to 100000 */
                                   MAType        optInSlowMAType,
                                   int           optInSignalPeriod, /* From 1 to 100000 */
                                   MAType        optInSignalMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         { return MacdExt( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<double>(inReal,0),
                optInFastPeriod, /* From 2 to 100000 */
                optInFastMAType,
                optInSlowPeriod, /* From 2 to 100000 */
                optInSlowMAType,
                optInSignalPeriod, /* From 1 to 100000 */
                optInSignalMAType,
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outMACD,0),
                  gcnew SubArrayFrom1D<double>(outMACDSignal,0),
                  gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         { return MacdExt( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<float>(inReal,0),
                optInFastPeriod, /* From 2 to 100000 */
                optInFastMAType,
                optInSlowPeriod, /* From 2 to 100000 */
                optInSlowMAType,
                optInSignalPeriod, /* From 1 to 100000 */
                optInSignalMAType,
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outMACD,0),
                  gcnew SubArrayFrom1D<double>(outMACDSignal,0),
                  gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
         static enum class RetCode MacdExt( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInFastPeriod, /* From 2 to 100000 */
                                            MAType        optInFastMAType,
                                            int           optInSlowPeriod, /* From 2 to 100000 */
                                            MAType        optInSlowMAType,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            MAType        optInSignalMAType,
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
         #endif

         #define TA_MACDEXT Core::MacdExt
         #define TA_MACDEXT_Lookback Core::MacdExtLookback

         static int MacdFixLookback( int           optInSignalPeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outMACD,
                                            SubArray<double>^  outMACDSignal,
                                            SubArray<double>^  outMACDHist );

         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         { return MacdFix( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<double>(inReal,0),
                optInSignalPeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outMACD,0),
                  gcnew SubArrayFrom1D<double>(outMACDSignal,0),
                  gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist )
         { return MacdFix( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<float>(inReal,0),
                optInSignalPeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outMACD,0),
                  gcnew SubArrayFrom1D<double>(outMACDSignal,0),
                  gcnew SubArrayFrom1D<double>(outMACDHist,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
         static enum class RetCode MacdFix( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInSignalPeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outMACD,
                                            cli::array<double>^  outMACDSignal,
                                            cli::array<double>^  outMACDHist );
         #endif

         #define TA_MACDFIX Core::MacdFix
         #define TA_MACDFIX_Lookback Core::MacdFixLookback

         static int MamaLookback( double        optInFastLimit, /* From 0.01 to 0.99 */
                                double        optInSlowLimit );  /* From 0.01 to 0.99 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMAMA,
                                         SubArray<double>^  outFAMA );

         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outMAMA,
                                         SubArray<double>^  outFAMA );

         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA )
         { return Mama( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInFastLimit, /* From 0.01 to 0.99 */
              optInSlowLimit, /* From 0.01 to 0.99 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outMAMA,0),
                gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA )
         { return Mama( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInFastLimit, /* From 0.01 to 0.99 */
              optInSlowLimit, /* From 0.01 to 0.99 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outMAMA,0),
                gcnew SubArrayFrom1D<double>(outFAMA,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA );
         static enum class RetCode Mama( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         double        optInFastLimit, /* From 0.01 to 0.99 */
                                         double        optInSlowLimit, /* From 0.01 to 0.99 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outMAMA,
                                         cli::array<double>^  outFAMA );
         #endif

         #define TA_MAMA Core::Mama
         #define TA_MAMA_Lookback Core::MamaLookback

         static int MovingAverageVariablePeriodLookback( int           optInMinPeriod, /* From 2 to 100000 */
                                                       int           optInMaxPeriod, /* From 2 to 100000 */
                                                       MAType        optInMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                SubArray<double>^ inReal,
                                                                SubArray<double>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                SubArray<double>^  outReal );

         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                SubArray<float>^ inReal,
                                                                SubArray<float>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                SubArray<double>^  outReal );

         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                cli::array<double>^ inReal,
                                                                cli::array<double>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                cli::array<double>^  outReal )
         { return MovingAverageVariablePeriod( startIdx,                       endIdx,
                                                gcnew SubArrayFrom1D<double>(inReal,0),
                                                gcnew SubArrayFrom1D<double>(inPeriods,0),
                                    optInMinPeriod, /* From 2 to 100000 */
                                    optInMaxPeriod, /* From 2 to 100000 */
                                    optInMAType,
                                   outBegIdx,
                                   outNBElement,
                                      gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                cli::array<float>^ inReal,
                                                                cli::array<float>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                cli::array<double>^  outReal )
         { return MovingAverageVariablePeriod( startIdx,                       endIdx,
                                                gcnew SubArrayFrom1D<float>(inReal,0),
                                                gcnew SubArrayFrom1D<float>(inPeriods,0),
                                    optInMinPeriod, /* From 2 to 100000 */
                                    optInMaxPeriod, /* From 2 to 100000 */
                                    optInMAType,
                                   outBegIdx,
                                   outNBElement,
                                      gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                cli::array<double>^ inReal,
                                                                cli::array<double>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                cli::array<double>^  outReal );
         static enum class RetCode MovingAverageVariablePeriod( int    startIdx,
                                                                int    endIdx,
                                                                cli::array<float>^ inReal,
                                                                cli::array<float>^ inPeriods,
                                                                int           optInMinPeriod, /* From 2 to 100000 */
                                                                int           optInMaxPeriod, /* From 2 to 100000 */
                                                                MAType        optInMAType,
                                                                [Out]int%    outBegIdx,
                                                                [Out]int%    outNBElement,
                                                                cli::array<double>^  outReal );
         #endif

         #define TA_MAVP Core::MovingAverageVariablePeriod
         #define TA_MAVP_Lookback Core::MovingAverageVariablePeriodLookback

         static int MaxLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Max( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Max( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Max( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_MAX Core::Max
         #define TA_MAX_Lookback Core::MaxLookback

         static int MaxIndexLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<int>^  outInteger );

         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<int>^  outInteger );

         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger )
         { return MaxIndex( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger )
         { return MaxIndex( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger );
         static enum class RetCode MaxIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger );
         #endif

         #define TA_MAXINDEX Core::MaxIndex
         #define TA_MAXINDEX_Lookback Core::MaxIndexLookback

         static int MedPriceLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MedPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MedPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode MedPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_MEDPRICE Core::MedPrice
         #define TA_MEDPRICE_Lookback Core::MedPriceLookback

         static int MfiLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        SubArray<double>^ inClose,
                                        SubArray<double>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        SubArray<float>^ inClose,
                                        SubArray<float>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        cli::array<double>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Mfi( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              gcnew SubArrayFrom1D<double>(inVolume,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        cli::array<float>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Mfi( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              gcnew SubArrayFrom1D<float>(inVolume,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        cli::array<double>^ inClose,
                                        cli::array<double>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Mfi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        cli::array<float>^ inClose,
                                        cli::array<float>^ inVolume,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_MFI Core::Mfi
         #define TA_MFI_Lookback Core::MfiLookback

         static int MidPointLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MidPoint( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MidPoint( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode MidPoint( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_MIDPOINT Core::MidPoint
         #define TA_MIDPOINT_Lookback Core::MidPointLookback

         static int MidPriceLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MidPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return MidPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode MidPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_MIDPRICE Core::MidPrice
         #define TA_MIDPRICE_Lookback Core::MidPriceLookback

         static int MinLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Min( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Min( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Min( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_MIN Core::Min
         #define TA_MIN_Lookback Core::MinLookback

         static int MinIndexLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<int>^  outInteger );

         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<int>^  outInteger );

         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger )
         { return MinIndex( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger )
         { return MinIndex( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<int>(outInteger,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger );
         static enum class RetCode MinIndex( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<int>^  outInteger );
         #endif

         #define TA_MININDEX Core::MinIndex
         #define TA_MININDEX_Lookback Core::MinIndexLookback

         static int MinMaxLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outMin,
                                           SubArray<double>^  outMax );

         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outMin,
                                           SubArray<double>^  outMax );

         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax )
         { return MinMax( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outMin,0),
                 gcnew SubArrayFrom1D<double>(outMax,0) );
         }
         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax )
         { return MinMax( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outMin,0),
                 gcnew SubArrayFrom1D<double>(outMax,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax );
         static enum class RetCode MinMax( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outMin,
                                           cli::array<double>^  outMax );
         #endif

         #define TA_MINMAX Core::MinMax
         #define TA_MINMAX_Lookback Core::MinMaxLookback

         static int MinMaxIndexLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                SubArray<double>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outMinIdx,
                                                SubArray<int>^  outMaxIdx );

         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                SubArray<float>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                SubArray<int>^  outMinIdx,
                                                SubArray<int>^  outMaxIdx );

         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx )
         { return MinMaxIndex( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<double>(inReal,0),
                    optInTimePeriod, /* From 2 to 100000 */
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outMinIdx,0),
                      gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx )
         { return MinMaxIndex( startIdx,       endIdx,
                                gcnew SubArrayFrom1D<float>(inReal,0),
                    optInTimePeriod, /* From 2 to 100000 */
                   outBegIdx,
                   outNBElement,
                      gcnew SubArrayFrom1D<int>(outMinIdx,0),
                      gcnew SubArrayFrom1D<int>(outMaxIdx,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                cli::array<double>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx );
         static enum class RetCode MinMaxIndex( int    startIdx,
                                                int    endIdx,
                                                cli::array<float>^ inReal,
                                                int           optInTimePeriod, /* From 2 to 100000 */
                                                [Out]int%    outBegIdx,
                                                [Out]int%    outNBElement,
                                                cli::array<int>^  outMinIdx,
                                                cli::array<int>^  outMaxIdx );
         #endif

         #define TA_MINMAXINDEX Core::MinMaxIndex
         #define TA_MINMAXINDEX_Lookback Core::MinMaxIndexLookback

         static int MinusDILookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inHigh,
                                            SubArray<double>^ inLow,
                                            SubArray<double>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inHigh,
                                            SubArray<float>^ inLow,
                                            SubArray<float>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            cli::array<double>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return MinusDI( startIdx,   endIdx,
                gcnew SubArrayFrom1D<double>(inHigh,0),
                gcnew SubArrayFrom1D<double>(inLow,0),
                gcnew SubArrayFrom1D<double>(inClose,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            cli::array<float>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return MinusDI( startIdx,   endIdx,
                gcnew SubArrayFrom1D<float>(inHigh,0),
                gcnew SubArrayFrom1D<float>(inLow,0),
                gcnew SubArrayFrom1D<float>(inClose,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            cli::array<double>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         static enum class RetCode MinusDI( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            cli::array<float>^ inClose,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         #endif

         #define TA_MINUS_DI Core::MinusDI
         #define TA_MINUS_DI_Lookback Core::MinusDILookback

         static int MinusDMLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inHigh,
                                            SubArray<double>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inHigh,
                                            SubArray<float>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return MinusDM( startIdx,   endIdx,
                gcnew SubArrayFrom1D<double>(inHigh,0),
                gcnew SubArrayFrom1D<double>(inLow,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return MinusDM( startIdx,   endIdx,
                gcnew SubArrayFrom1D<float>(inHigh,0),
                gcnew SubArrayFrom1D<float>(inLow,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inHigh,
                                            cli::array<double>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         static enum class RetCode MinusDM( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inHigh,
                                            cli::array<float>^ inLow,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         #endif

         #define TA_MINUS_DM Core::MinusDM
         #define TA_MINUS_DM_Lookback Core::MinusDMLookback

         static int MomLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Mom( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Mom( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Mom( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_MOM Core::Mom
         #define TA_MOM_Lookback Core::MomLookback

         static int MultLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal0,
                                         SubArray<double>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal0,
                                         SubArray<float>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal0,
                                         cli::array<double>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Mult( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal0,0),
                          gcnew SubArrayFrom1D<double>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal0,
                                         cli::array<float>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Mult( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal0,0),
                          gcnew SubArrayFrom1D<float>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal0,
                                         cli::array<double>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Mult( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal0,
                                         cli::array<float>^ inReal1,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_MULT Core::Mult
         #define TA_MULT_Lookback Core::MultLookback

         static int NatrLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inHigh,
                                         SubArray<double>^ inLow,
                                         SubArray<double>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inHigh,
                                         SubArray<float>^ inLow,
                                         SubArray<float>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inHigh,
                                         cli::array<double>^ inLow,
                                         cli::array<double>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Natr( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inHigh,
                                         cli::array<float>^ inLow,
                                         cli::array<float>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Natr( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inHigh,
                                         cli::array<double>^ inLow,
                                         cli::array<double>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Natr( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inHigh,
                                         cli::array<float>^ inLow,
                                         cli::array<float>^ inClose,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_NATR Core::Natr
         #define TA_NATR_Lookback Core::NatrLookback

         static int ObvLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        SubArray<double>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        SubArray<float>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        cli::array<double>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Obv( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              gcnew SubArrayFrom1D<double>(inVolume,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        cli::array<float>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Obv( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              gcnew SubArrayFrom1D<float>(inVolume,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        cli::array<double>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Obv( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        cli::array<float>^ inVolume,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_OBV Core::Obv
         #define TA_OBV_Lookback Core::ObvLookback

         static int PlusDILookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           SubArray<double>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           SubArray<float>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return PlusDI( startIdx,  endIdx,
               gcnew SubArrayFrom1D<double>(inHigh,0),
               gcnew SubArrayFrom1D<double>(inLow,0),
               gcnew SubArrayFrom1D<double>(inClose,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return PlusDI( startIdx,  endIdx,
               gcnew SubArrayFrom1D<float>(inHigh,0),
               gcnew SubArrayFrom1D<float>(inLow,0),
               gcnew SubArrayFrom1D<float>(inClose,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode PlusDI( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_PLUS_DI Core::PlusDI
         #define TA_PLUS_DI_Lookback Core::PlusDILookback

         static int PlusDMLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return PlusDM( startIdx,  endIdx,
               gcnew SubArrayFrom1D<double>(inHigh,0),
               gcnew SubArrayFrom1D<double>(inLow,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return PlusDM( startIdx,  endIdx,
               gcnew SubArrayFrom1D<float>(inHigh,0),
               gcnew SubArrayFrom1D<float>(inLow,0),
               optInTimePeriod, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode PlusDM( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           int           optInTimePeriod, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_PLUS_DM Core::PlusDM
         #define TA_PLUS_DM_Lookback Core::PlusDMLookback

         static int PpoLookback( int           optInFastPeriod, /* From 2 to 100000 */
                               int           optInSlowPeriod, /* From 2 to 100000 */
                               MAType        optInMAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Ppo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInMAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Ppo( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInFastPeriod, /* From 2 to 100000 */
              optInSlowPeriod, /* From 2 to 100000 */
              optInMAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Ppo( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInFastPeriod, /* From 2 to 100000 */
                                        int           optInSlowPeriod, /* From 2 to 100000 */
                                        MAType        optInMAType,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_PPO Core::Ppo
         #define TA_PPO_Lookback Core::PpoLookback

         static int RocLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Roc( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Roc( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Roc( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 1 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_ROC Core::Roc
         #define TA_ROC_Lookback Core::RocLookback

         static int RocPLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return RocP( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return RocP( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode RocP( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ROCP Core::RocP
         #define TA_ROCP_Lookback Core::RocPLookback

         static int RocRLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return RocR( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return RocR( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode RocR( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_ROCR Core::RocR
         #define TA_ROCR_Lookback Core::RocRLookback

         static int RocR100Lookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            SubArray<double>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            SubArray<float>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            SubArray<double>^  outReal );

         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return RocR100( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<double>(inReal,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal )
         { return RocR100( startIdx,   endIdx,
                            gcnew SubArrayFrom1D<float>(inReal,0),
                optInTimePeriod, /* From 1 to 100000 */
               outBegIdx,
               outNBElement,
                  gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            cli::array<double>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         static enum class RetCode RocR100( int    startIdx,
                                            int    endIdx,
                                            cli::array<float>^ inReal,
                                            int           optInTimePeriod, /* From 1 to 100000 */
                                            [Out]int%    outBegIdx,
                                            [Out]int%    outNBElement,
                                            cli::array<double>^  outReal );
         #endif

         #define TA_ROCR100 Core::RocR100
         #define TA_ROCR100_Lookback Core::RocR100Lookback

         static int RsiLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Rsi( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Rsi( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Rsi( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_RSI Core::Rsi
         #define TA_RSI_Lookback Core::RsiLookback

         static int SarLookback( double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                               double        optInMaximum );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inHigh,
                                        SubArray<double>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inHigh,
                                        SubArray<float>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sar( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              optInAcceleration, /* From 0 to TA_REAL_MAX */
              optInMaximum, /* From 0 to TA_REAL_MAX */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sar( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              optInAcceleration, /* From 0 to TA_REAL_MAX */
              optInMaximum, /* From 0 to TA_REAL_MAX */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inHigh,
                                        cli::array<double>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Sar( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inHigh,
                                        cli::array<float>^ inLow,
                                        double        optInAcceleration, /* From 0 to TA_REAL_MAX */
                                        double        optInMaximum, /* From 0 to TA_REAL_MAX */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_SAR Core::Sar
         #define TA_SAR_Lookback Core::SarLookback

         static int SarExtLookback( double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                  double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                  double        optInAccelerationMaxShort );  /* From 0 to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return SarExt( startIdx,  endIdx,
               gcnew SubArrayFrom1D<double>(inHigh,0),
               gcnew SubArrayFrom1D<double>(inLow,0),
               optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
               optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
               optInAccelerationShort, /* From 0 to TA_REAL_MAX */
               optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return SarExt( startIdx,  endIdx,
               gcnew SubArrayFrom1D<float>(inHigh,0),
               gcnew SubArrayFrom1D<float>(inLow,0),
               optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
               optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
               optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
               optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
               optInAccelerationShort, /* From 0 to TA_REAL_MAX */
               optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode SarExt( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           double        optInStartValue, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           double        optInOffsetOnReverse, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxLong, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationInitShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationShort, /* From 0 to TA_REAL_MAX */
                                           double        optInAccelerationMaxShort, /* From 0 to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_SAREXT Core::SarExt
         #define TA_SAREXT_Lookback Core::SarExtLookback

         static int SinLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sin( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sin( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Sin( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_SIN Core::Sin
         #define TA_SIN_Lookback Core::SinLookback

         static int SinhLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Sinh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Sinh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Sinh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_SINH Core::Sinh
         #define TA_SINH_Lookback Core::SinhLookback

         static int SmaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sma( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sma( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Sma( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_SMA Core::Sma
         #define TA_SMA_Lookback Core::SmaLookback

         static int SqrtLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Sqrt( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Sqrt( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Sqrt( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_SQRT Core::Sqrt
         #define TA_SQRT_Lookback Core::SqrtLookback

         static int StdDevLookback( int           optInTimePeriod, /* From 2 to 100000 */
                                  double        optInNbDev );  /* From TA_REAL_MIN to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return StdDev( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<double>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
               optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return StdDev( startIdx,  endIdx,
                           gcnew SubArrayFrom1D<float>(inReal,0),
               optInTimePeriod, /* From 2 to 100000 */
               optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode StdDev( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inReal,
                                           int           optInTimePeriod, /* From 2 to 100000 */
                                           double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_STDDEV Core::StdDev
         #define TA_STDDEV_Lookback Core::StdDevLookback

         static int StochLookback( int           optInFastK_Period, /* From 1 to 100000 */
                                 int           optInSlowK_Period, /* From 1 to 100000 */
                                 MAType        optInSlowK_MAType,
                                 int           optInSlowD_Period, /* From 1 to 100000 */
                                 MAType        optInSlowD_MAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inHigh,
                                          SubArray<double>^ inLow,
                                          SubArray<double>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outSlowK,
                                          SubArray<double>^  outSlowD );

         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inHigh,
                                          SubArray<float>^ inLow,
                                          SubArray<float>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outSlowK,
                                          SubArray<double>^  outSlowD );

         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outSlowK,
                                          cli::array<double>^  outSlowD )
         { return Stoch( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInFastK_Period, /* From 1 to 100000 */
              optInSlowK_Period, /* From 1 to 100000 */
              optInSlowK_MAType,
              optInSlowD_Period, /* From 1 to 100000 */
              optInSlowD_MAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outSlowK,0),
                gcnew SubArrayFrom1D<double>(outSlowD,0) );
         }
         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outSlowK,
                                          cli::array<double>^  outSlowD )
         { return Stoch( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInFastK_Period, /* From 1 to 100000 */
              optInSlowK_Period, /* From 1 to 100000 */
              optInSlowK_MAType,
              optInSlowD_Period, /* From 1 to 100000 */
              optInSlowD_MAType,
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outSlowK,0),
                gcnew SubArrayFrom1D<double>(outSlowD,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outSlowK,
                                          cli::array<double>^  outSlowD );
         static enum class RetCode Stoch( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          int           optInFastK_Period, /* From 1 to 100000 */
                                          int           optInSlowK_Period, /* From 1 to 100000 */
                                          MAType        optInSlowK_MAType,
                                          int           optInSlowD_Period, /* From 1 to 100000 */
                                          MAType        optInSlowD_MAType,
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outSlowK,
                                          cli::array<double>^  outSlowD );
         #endif

         #define TA_STOCH Core::Stoch
         #define TA_STOCH_Lookback Core::StochLookback

         static int StochFLookback( int           optInFastK_Period, /* From 1 to 100000 */
                                  int           optInFastD_Period, /* From 1 to 100000 */
                                  MAType        optInFastD_MAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           SubArray<double>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outFastK,
                                           SubArray<double>^  outFastD );

         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           SubArray<float>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outFastK,
                                           SubArray<double>^  outFastD );

         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD )
         { return StochF( startIdx,  endIdx,
               gcnew SubArrayFrom1D<double>(inHigh,0),
               gcnew SubArrayFrom1D<double>(inLow,0),
               gcnew SubArrayFrom1D<double>(inClose,0),
               optInFastK_Period, /* From 1 to 100000 */
               optInFastD_Period, /* From 1 to 100000 */
               optInFastD_MAType,
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outFastK,0),
                 gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD )
         { return StochF( startIdx,  endIdx,
               gcnew SubArrayFrom1D<float>(inHigh,0),
               gcnew SubArrayFrom1D<float>(inLow,0),
               gcnew SubArrayFrom1D<float>(inClose,0),
               optInFastK_Period, /* From 1 to 100000 */
               optInFastD_Period, /* From 1 to 100000 */
               optInFastD_MAType,
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outFastK,0),
                 gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD );
         static enum class RetCode StochF( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInFastK_Period, /* From 1 to 100000 */
                                           int           optInFastD_Period, /* From 1 to 100000 */
                                           MAType        optInFastD_MAType,
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outFastK,
                                           cli::array<double>^  outFastD );
         #endif

         #define TA_STOCHF Core::StochF
         #define TA_STOCHF_Lookback Core::StochFLookback

         static int StochRsiLookback( int           optInTimePeriod, /* From 2 to 100000 */
                                    int           optInFastK_Period, /* From 1 to 100000 */
                                    int           optInFastD_Period, /* From 1 to 100000 */
                                    MAType        optInFastD_MAType ); 
         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outFastK,
                                             SubArray<double>^  outFastD );

         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outFastK,
                                             SubArray<double>^  outFastD );

         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD )
         { return StochRsi( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                 optInFastK_Period, /* From 1 to 100000 */
                 optInFastD_Period, /* From 1 to 100000 */
                 optInFastD_MAType,
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outFastK,0),
                   gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD )
         { return StochRsi( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                 optInTimePeriod, /* From 2 to 100000 */
                 optInFastK_Period, /* From 1 to 100000 */
                 optInFastD_Period, /* From 1 to 100000 */
                 optInFastD_MAType,
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outFastK,0),
                   gcnew SubArrayFrom1D<double>(outFastD,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD );
         static enum class RetCode StochRsi( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 2 to 100000 */
                                             int           optInFastK_Period, /* From 1 to 100000 */
                                             int           optInFastD_Period, /* From 1 to 100000 */
                                             MAType        optInFastD_MAType,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outFastK,
                                             cli::array<double>^  outFastD );
         #endif

         #define TA_STOCHRSI Core::StochRsi
         #define TA_STOCHRSI_Lookback Core::StochRsiLookback

         static int SubLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal0,
                                        SubArray<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal0,
                                        SubArray<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sub( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal0,0),
                          gcnew SubArrayFrom1D<double>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sub( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal0,0),
                          gcnew SubArrayFrom1D<float>(inReal1,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal0,
                                        cli::array<double>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Sub( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal0,
                                        cli::array<float>^ inReal1,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_SUB Core::Sub
         #define TA_SUB_Lookback Core::SubLookback

         static int SumLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sum( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Sum( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Sum( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_SUM Core::Sum
         #define TA_SUM_Lookback Core::SumLookback

         static int T3Lookback( int           optInTimePeriod, /* From 2 to 100000 */
                              double        optInVFactor );  /* From 0 to 1 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       SubArray<double>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       SubArray<float>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       SubArray<double>^  outReal );

         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return T3( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
              optInVFactor, /* From 0 to 1 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal )
         { return T3( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
              optInVFactor, /* From 0 to 1 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       cli::array<double>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         static enum class RetCode T3( int    startIdx,
                                       int    endIdx,
                                       cli::array<float>^ inReal,
                                       int           optInTimePeriod, /* From 2 to 100000 */
                                       double        optInVFactor, /* From 0 to 1 */
                                       [Out]int%    outBegIdx,
                                       [Out]int%    outNBElement,
                                       cli::array<double>^  outReal );
         #endif

         #define TA_T3 Core::T3
         #define TA_T3_Lookback Core::T3Lookback

         static int TanLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Tan( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Tan( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Tan( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_TAN Core::Tan
         #define TA_TAN_Lookback Core::TanLookback

         static int TanhLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Tanh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Tanh( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Tanh( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_TANH Core::Tanh
         #define TA_TANH_Lookback Core::TanhLookback

         static int TemaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Tema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Tema( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Tema( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 2 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_TEMA Core::Tema
         #define TA_TEMA_Lookback Core::TemaLookback

         static int TrueRangeLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              SubArray<double>^ inHigh,
                                              SubArray<double>^ inLow,
                                              SubArray<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              SubArray<float>^ inHigh,
                                              SubArray<float>^ inLow,
                                              SubArray<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              SubArray<double>^  outReal );

         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return TrueRange( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<double>(inHigh,0),
                  gcnew SubArrayFrom1D<double>(inLow,0),
                  gcnew SubArrayFrom1D<double>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal )
         { return TrueRange( startIdx,     endIdx,
                  gcnew SubArrayFrom1D<float>(inHigh,0),
                  gcnew SubArrayFrom1D<float>(inLow,0),
                  gcnew SubArrayFrom1D<float>(inClose,0),
                 outBegIdx,
                 outNBElement,
                    gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              cli::array<double>^ inHigh,
                                              cli::array<double>^ inLow,
                                              cli::array<double>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         static enum class RetCode TrueRange( int    startIdx,
                                              int    endIdx,
                                              cli::array<float>^ inHigh,
                                              cli::array<float>^ inLow,
                                              cli::array<float>^ inClose,
                                              [Out]int%    outBegIdx,
                                              [Out]int%    outNBElement,
                                              cli::array<double>^  outReal );
         #endif

         #define TA_TRANGE Core::TrueRange
         #define TA_TRANGE_Lookback Core::TrueRangeLookback

         static int TrimaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Trima( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return Trima( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         static enum class RetCode Trima( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inReal,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         #endif

         #define TA_TRIMA Core::Trima
         #define TA_TRIMA_Lookback Core::TrimaLookback

         static int TrixLookback( int           optInTimePeriod );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         SubArray<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         SubArray<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         SubArray<double>^  outReal );

         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Trix( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal )
         { return Trix( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 1 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         cli::array<double>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         static enum class RetCode Trix( int    startIdx,
                                         int    endIdx,
                                         cli::array<float>^ inReal,
                                         int           optInTimePeriod, /* From 1 to 100000 */
                                         [Out]int%    outBegIdx,
                                         [Out]int%    outNBElement,
                                         cli::array<double>^  outReal );
         #endif

         #define TA_TRIX Core::Trix
         #define TA_TRIX_Lookback Core::TrixLookback

         static int TsfLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Tsf( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Tsf( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Tsf( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_TSF Core::Tsf
         #define TA_TSF_Lookback Core::TsfLookback

         static int TypPriceLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             SubArray<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             SubArray<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return TypPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 gcnew SubArrayFrom1D<double>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return TypPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 gcnew SubArrayFrom1D<float>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode TypPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_TYPPRICE Core::TypPrice
         #define TA_TYPPRICE_Lookback Core::TypPriceLookback

         static int UltOscLookback( int           optInTimePeriod1, /* From 1 to 100000 */
                                  int           optInTimePeriod2, /* From 1 to 100000 */
                                  int           optInTimePeriod3 );  /* From 1 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           SubArray<double>^ inHigh,
                                           SubArray<double>^ inLow,
                                           SubArray<double>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           SubArray<float>^ inHigh,
                                           SubArray<float>^ inLow,
                                           SubArray<float>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           SubArray<double>^  outReal );

         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return UltOsc( startIdx,  endIdx,
               gcnew SubArrayFrom1D<double>(inHigh,0),
               gcnew SubArrayFrom1D<double>(inLow,0),
               gcnew SubArrayFrom1D<double>(inClose,0),
               optInTimePeriod1, /* From 1 to 100000 */
               optInTimePeriod2, /* From 1 to 100000 */
               optInTimePeriod3, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal )
         { return UltOsc( startIdx,  endIdx,
               gcnew SubArrayFrom1D<float>(inHigh,0),
               gcnew SubArrayFrom1D<float>(inLow,0),
               gcnew SubArrayFrom1D<float>(inClose,0),
               optInTimePeriod1, /* From 1 to 100000 */
               optInTimePeriod2, /* From 1 to 100000 */
               optInTimePeriod3, /* From 1 to 100000 */
              outBegIdx,
              outNBElement,
                 gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           cli::array<double>^ inHigh,
                                           cli::array<double>^ inLow,
                                           cli::array<double>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         static enum class RetCode UltOsc( int    startIdx,
                                           int    endIdx,
                                           cli::array<float>^ inHigh,
                                           cli::array<float>^ inLow,
                                           cli::array<float>^ inClose,
                                           int           optInTimePeriod1, /* From 1 to 100000 */
                                           int           optInTimePeriod2, /* From 1 to 100000 */
                                           int           optInTimePeriod3, /* From 1 to 100000 */
                                           [Out]int%    outBegIdx,
                                           [Out]int%    outNBElement,
                                           cli::array<double>^  outReal );
         #endif

         #define TA_ULTOSC Core::UltOsc
         #define TA_ULTOSC_Lookback Core::UltOscLookback

         static int VarianceLookback( int           optInTimePeriod, /* From 1 to 100000 */
                                    double        optInNbDev );  /* From TA_REAL_MIN to TA_REAL_MAX */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return Variance( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<double>(inReal,0),
                 optInTimePeriod, /* From 1 to 100000 */
                 optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return Variance( startIdx,    endIdx,
                             gcnew SubArrayFrom1D<float>(inReal,0),
                 optInTimePeriod, /* From 1 to 100000 */
                 optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode Variance( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inReal,
                                             int           optInTimePeriod, /* From 1 to 100000 */
                                             double        optInNbDev, /* From TA_REAL_MIN to TA_REAL_MAX */
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_VAR Core::Variance
         #define TA_VAR_Lookback Core::VarianceLookback

         static int WclPriceLookback( void );

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<double>^ inHigh,
                                             SubArray<double>^ inLow,
                                             SubArray<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             SubArray<float>^ inHigh,
                                             SubArray<float>^ inLow,
                                             SubArray<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             SubArray<double>^  outReal );

         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return WclPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<double>(inHigh,0),
                 gcnew SubArrayFrom1D<double>(inLow,0),
                 gcnew SubArrayFrom1D<double>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal )
         { return WclPrice( startIdx,    endIdx,
                 gcnew SubArrayFrom1D<float>(inHigh,0),
                 gcnew SubArrayFrom1D<float>(inLow,0),
                 gcnew SubArrayFrom1D<float>(inClose,0),
                outBegIdx,
                outNBElement,
                   gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<double>^ inHigh,
                                             cli::array<double>^ inLow,
                                             cli::array<double>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         static enum class RetCode WclPrice( int    startIdx,
                                             int    endIdx,
                                             cli::array<float>^ inHigh,
                                             cli::array<float>^ inLow,
                                             cli::array<float>^ inClose,
                                             [Out]int%    outBegIdx,
                                             [Out]int%    outNBElement,
                                             cli::array<double>^  outReal );
         #endif

         #define TA_WCLPRICE Core::WclPrice
         #define TA_WCLPRICE_Lookback Core::WclPriceLookback

         static int WillRLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          SubArray<double>^ inHigh,
                                          SubArray<double>^ inLow,
                                          SubArray<double>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          SubArray<float>^ inHigh,
                                          SubArray<float>^ inLow,
                                          SubArray<float>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          SubArray<double>^  outReal );

         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return WillR( startIdx, endIdx,
              gcnew SubArrayFrom1D<double>(inHigh,0),
              gcnew SubArrayFrom1D<double>(inLow,0),
              gcnew SubArrayFrom1D<double>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal )
         { return WillR( startIdx, endIdx,
              gcnew SubArrayFrom1D<float>(inHigh,0),
              gcnew SubArrayFrom1D<float>(inLow,0),
              gcnew SubArrayFrom1D<float>(inClose,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          cli::array<double>^ inHigh,
                                          cli::array<double>^ inLow,
                                          cli::array<double>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         static enum class RetCode WillR( int    startIdx,
                                          int    endIdx,
                                          cli::array<float>^ inHigh,
                                          cli::array<float>^ inLow,
                                          cli::array<float>^ inClose,
                                          int           optInTimePeriod, /* From 2 to 100000 */
                                          [Out]int%    outBegIdx,
                                          [Out]int%    outNBElement,
                                          cli::array<double>^  outReal );
         #endif

         #define TA_WILLR Core::WillR
         #define TA_WILLR_Lookback Core::WillRLookback

         static int WmaLookback( int           optInTimePeriod );  /* From 2 to 100000 */

         #if defined( _MANAGED ) && defined( USE_SUBARRAY )
         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        SubArray<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        SubArray<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        SubArray<double>^  outReal );

         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Wma( startIdx, endIdx,
                          gcnew SubArrayFrom1D<double>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal )
         { return Wma( startIdx, endIdx,
                          gcnew SubArrayFrom1D<float>(inReal,0),
              optInTimePeriod, /* From 2 to 100000 */
             outBegIdx,
             outNBElement,
                gcnew SubArrayFrom1D<double>(outReal,0) );
         }
         #elif defined( _MANAGED )
         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        cli::array<double>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         static enum class RetCode Wma( int    startIdx,
                                        int    endIdx,
                                        cli::array<float>^ inReal,
                                        int           optInTimePeriod, /* From 2 to 100000 */
                                        [Out]int%    outBegIdx,
                                        [Out]int%    outNBElement,
                                        cli::array<double>^  outReal );
         #endif

         #define TA_WMA Core::Wma
         #define TA_WMA_Lookback Core::WmaLookback

/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/

  		   };
	   }
	}
}
