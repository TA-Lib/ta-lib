/* TA-LIB Copyright (c) 1999-2006, Mario Fortier
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
#ifndef TA_DEFS_H
#define TA_DEFS_H

/*** The following block of code is to define:
 ***
 ***    UInt32  : 32 bits unsigned integer.
 ***    Int32   : 32 bits signed integer.
 ***    UInt64  : 64 bits unsigned integer.
 ***    Int64   : 64 bits signed integer.
 ***
 ***    INT_MIN : The minimal value for Int32
 ***    INT_MAX : The maximal value for Int32
 ***/
#ifndef FD_DEFS_H
  #if defined( _MANAGED )
    /* Int32, UInt32, Int64 and UInt64 are built-in for .NET */	
    #define INT_MIN (Int32::MinValue)
    #define INT_MAX (Int32::MaxValue)
  #elif defined( _JAVA )
    #define INT_MIN Integer.MIN_VALUE
    #define INT_MAX Integer.MAX_VALUE
  #else
    #include <limits.h>

    /* Identify if 64 bits platform with __64BIT__.
     * Can also be done from compiler command line. 
     */
    #if defined(_WIN64)
       #define __64BIT__ 1
    #endif

    #if defined( __LP64__ ) || defined( _LP64 )
       #define __64BIT__ 1
    #endif

    /* Check also for some known GCC def for 64 bits platform. */
    #if defined(__alpha__)\
        ||defined(__ia64__)\
        ||defined(__ppc64__)\
        ||defined(__s390x__)\
        ||defined(__x86_64__)
       #define __64BIT__ 1
    #endif		  
		   
    #if !defined(__MACTYPES__)
        typedef signed int   Int32;
        typedef unsigned int UInt32;

        #if defined(_WIN32) || defined(_WIN64)
           /* See "Windows Data Types". Platform SDK. MSDN documentation. */
           typedef signed __int64   Int64;
           typedef unsigned __int64 UInt64;
        #else
           #if defined(__64BIT__)
              /* Standard LP64 model for 64 bits Unix platform. */
              typedef signed long   Int64;
              typedef unsigned long UInt64;
           #else
              /* Standard ILP32 model for 32 bits Unix platform. */
              typedef signed long long   Int64;
              typedef unsigned long long UInt64;
           #endif
         #endif
    #endif
  #endif
#endif

/* Enumeration and macros to abstract from syntax difference 
 * between C, C++, managed C++ and Java.
 */
#if defined( _MANAGED )
  #define ENUM_BEGIN(x) enum class x {
  #define ENUM_END(x) };  
  #define ENUM_CASE(namespace,value) namespace::value

  #define STRUCT_BEGIN(x) struct x {
  #define STRUCT_END(x) };
  #define NAMESPACE(x) x::

  #define UNUSED_VARIABLE(x)             (void)x

  #define VALUE_HANDLE_INT(name)           int name
  #define VALUE_HANDLE_DEREF(name)         name
  #define VALUE_HANDLE_DEREF_TO_ZERO(name) name = 0
  #define VALUE_HANDLE_OUT(name)           name

  #define VALUE_HANDLE_GET(name)         name
  #define VALUE_HANDLE_SET(name,x)       name = x

  #define CONSTANT_DOUBLE(x) const double x

  #define FUNCTION_CALL(x) x
  #define FUNCTION_CALL_DOUBLE(x) x
  #define LOOKBACK_CALL(x) x##_Lookback
#elif defined( _JAVA )
  #define ENUM_BEGIN(x) public enum x {
  #define ENUM_END(x) };
  #define ENUM_CASE(namespace,value) value

  #define STRUCT_BEGIN(x) public class x {
  #define STRUCT_END(x) };
  #define NAMESPACE(x) x.

  #define UNUSED_VARIABLE(x)

  #define VALUE_HANDLE_INT(name)            MInteger name = new MInteger()
  #define VALUE_HANDLE_DEREF(name)          name.value
  #define VALUE_HANDLE_DEREF_TO_ZERO(name)  name.value = 0
  #define VALUE_HANDLE_OUT(name)            name

  #define VALUE_HANDLE_GET(name)         name.value
  #define VALUE_HANDLE_SET(name,x)       name.value = x

  #define CONSTANT_DOUBLE(x) final double x

  #define FUNCTION_CALL(x) x
  #define FUNCTION_CALL_DOUBLE(x) x
  #define LOOKBACK_CALL(x) x##_Lookback
#else
  #define ENUM_BEGIN(x) typedef enum {
  #define ENUM_END(x) } x;
  #define ENUM_CASE(namespace,value) value

  #define STRUCT_BEGIN(x) typedef struct {
  #define STRUCT_END(x) } x;
  #define NAMESPACE(x)

  #define UNUSED_VARIABLE(x)              (void)x

  #define VALUE_HANDLE_INT(name)           int name
  #define VALUE_HANDLE_DEREF(name)         (*name)
  #define VALUE_HANDLE_DEREF_TO_ZERO(name) (*name) = 0
  #define VALUE_HANDLE_OUT(name)           &name

  #define VALUE_HANDLE_GET(name)          name
  #define VALUE_HANDLE_SET(name,x)        name = x

  #define CONSTANT_DOUBLE(x) const double x

  #define FUNCTION_CALL(x) TA_PREFIX(x)
  #define FUNCTION_CALL_DOUBLE(x) TA_##x
  #define LOOKBACK_CALL(x) TA_##x##_Lookback
#endif

/* min/max value for a TA_Integer */
#define TA_INTEGER_MIN (INT_MIN+1)
#define TA_INTEGER_MAX (INT_MAX)

/* min/max value for a TA_Real 
 *
 * Use fix value making sense in the
 * context of TA-Lib (avoid to use DBL_MIN
 * and DBL_MAX standard macro because they
 * are troublesome with some compiler).
 */
#define TA_REAL_MIN (-3e+37)
#define TA_REAL_MAX (3e+37)

/* A value outside of the min/max range 
 * indicates an undefined or default value.
 */
#define TA_INTEGER_DEFAULT (INT_MIN)
#define TA_REAL_DEFAULT    (-4e+37)

/* Part of this file is generated by gen_code */
ENUM_BEGIN(TA_RetCode)
    /****** TA-LIB Error Code *****/
    /*   0 */  TA_SUCCESS,            /* No error */
    /*   1 */  TA_LIB_NOT_INITIALIZE, /* TA_Initialize was not sucessfully called */
    /*   2 */  TA_BAD_PARAM,          /* A parameter is out of range */
    /*   3 */  TA_ALLOC_ERR,          /* Possibly out-of-memory */
    /*   4 */  TA_GROUP_NOT_FOUND,
    /*   5 */  TA_FUNC_NOT_FOUND,
    /*   6 */  TA_INVALID_HANDLE,
    /*   7 */  TA_INVALID_PARAM_HOLDER,
    /*   8 */  TA_INVALID_PARAM_HOLDER_TYPE,
    /*   9 */  TA_INVALID_PARAM_FUNCTION,
    /*  10 */  TA_INPUT_NOT_ALL_INITIALIZE,
    /*  11 */  TA_OUTPUT_NOT_ALL_INITIALIZE,
    /*  12 */  TA_OUT_OF_RANGE_START_INDEX,
    /*  13 */  TA_OUT_OF_RANGE_END_INDEX,
    /*  14 */  TA_MEM_LEAK,
    /*  15 */  TA_FATAL_ERR,
    /*  16 */  TA_INVALID_LIST_TYPE,
    /*  17 */  TA_BAD_OBJECT,
    /*  18 */  TA_NOT_SUPPORTED,

    /* 5000 */ TA_INTERNAL_ERROR = 5000, /* Internal Error - Contact TA-Lib.org */

    /****** TA-Lib Contributors: See ta_trace.h for the TA_INTERNAL_ERROR macro *****/
    TA_UNKNOWN_ERR = 0xFFFF
ENUM_END(TA_RetCode)

ENUM_BEGIN( TA_Compatibility )
    TA_COMPATIBILITY_DEFAULT,
    TA_COMPATIBILITY_METASTOCK
ENUM_END( TA_Compatibility )

/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ ENUM_BEGIN( TA_MAType )
/* Generated */    TA_MAType_SMA       =0,
/* Generated */    TA_MAType_EMA       =1,
/* Generated */    TA_MAType_WMA       =2,
/* Generated */    TA_MAType_DEMA      =3,
/* Generated */    TA_MAType_TEMA      =4,
/* Generated */    TA_MAType_TRIMA     =5,
/* Generated */    TA_MAType_KAMA      =6,
/* Generated */    TA_MAType_MAMA      =7,
/* Generated */    TA_MAType_T3        =8
/* Generated */ ENUM_END( TA_MAType )
/* Generated */ 
/* Generated */ ENUM_BEGIN( TA_FuncUnstId )
/* Generated */     /* 000 */  TA_FUNC_UNST_ADX,
/* Generated */     /* 001 */  TA_FUNC_UNST_ADXR,
/* Generated */     /* 002 */  TA_FUNC_UNST_ATR,
/* Generated */     /* 003 */  TA_FUNC_UNST_CMO,
/* Generated */     /* 004 */  TA_FUNC_UNST_DX,
/* Generated */     /* 005 */  TA_FUNC_UNST_EMA,
/* Generated */     /* 006 */  TA_FUNC_UNST_HT_DCPERIOD,
/* Generated */     /* 007 */  TA_FUNC_UNST_HT_DCPHASE,
/* Generated */     /* 008 */  TA_FUNC_UNST_HT_PHASOR,
/* Generated */     /* 009 */  TA_FUNC_UNST_HT_SINE,
/* Generated */     /* 010 */  TA_FUNC_UNST_HT_TRENDLINE,
/* Generated */     /* 011 */  TA_FUNC_UNST_HT_TRENDMODE,
/* Generated */     /* 012 */  TA_FUNC_UNST_KAMA,
/* Generated */     /* 013 */  TA_FUNC_UNST_MAMA,
/* Generated */     /* 014 */  TA_FUNC_UNST_MFI,
/* Generated */     /* 015 */  TA_FUNC_UNST_MINUS_DI,
/* Generated */     /* 016 */  TA_FUNC_UNST_MINUS_DM,
/* Generated */     /* 017 */  TA_FUNC_UNST_NATR,
/* Generated */     /* 018 */  TA_FUNC_UNST_PLUS_DI,
/* Generated */     /* 019 */  TA_FUNC_UNST_PLUS_DM,
/* Generated */     /* 020 */  TA_FUNC_UNST_RSI,
/* Generated */     /* 021 */  TA_FUNC_UNST_STOCHRSI,
/* Generated */     /* 022 */  TA_FUNC_UNST_T3,
/* Generated */                TA_FUNC_UNST_ALL,
/* Generated */                TA_FUNC_UNST_NONE=-1
/* Generated */ ENUM_END( TA_FuncUnstId )
/* Generated */ 
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/

/* The TA_RangeType enum specifies the types of range that can be considered 
 * when to compare a part of a candle to other candles
 */

ENUM_BEGIN( TA_RangeType )
   TA_RangeType_RealBody,
   TA_RangeType_HighLow,
   TA_RangeType_Shadows
ENUM_END( TA_RangeType )

/* The TA_CandleSettingType enum specifies which kind of setting to consider;
 * the settings are based on the parts of the candle and the common words
 * indicating the length (short, long, very long)
 */
ENUM_BEGIN( TA_CandleSettingType )
    TA_BodyLong,
    TA_BodyVeryLong,
    TA_BodyShort,
    TA_BodyDoji,
    TA_ShadowLong,
    TA_ShadowVeryLong,
    TA_ShadowShort,
    TA_ShadowVeryShort,
    TA_Near,
    TA_Far,
    TA_Equal,
    TA_AllCandleSettings
ENUM_END( TA_CandleSettingType )

#endif
