CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

PROJECT(ta-lib)

set(TA_LIB_VERSION_MAJOR 0)
set(TA_LIB_VERSION_MINOR 6)
set(TA_LIB_VERSION_BUILD 0)

SET(TA_LIB_VERSION_FULL "${TA_LIB_VERSION_MAJOR}.${TA_LIB_VERSION_MINOR}.${TA_LIB_VERSION_BUILD}")

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckSymbolExists)
CHECK_INCLUDE_FILES(float.h HAVE_FLOAT_H)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(limits.h HAVE_LIMITS_H)
CHECK_INCLUDE_FILES(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILES(stddef.h HAVE_STDDEF_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(wchar.h HAVE_WCHAR_H)
CHECK_INCLUDE_FILES(wctype.h HAVE_WCTYPE_H)
CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(isascii HAVE_ISASCII)
CHECK_FUNCTION_EXISTS(localeconv HAVE_LOCALECONV)
CHECK_FUNCTION_EXISTS(mblen HAVE_MBLEN)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(modf HAVE_MODF)
CHECK_FUNCTION_EXISTS(pow HAVE_POW)
CHECK_FUNCTION_EXISTS(sqrt HAVE_SQRT)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strncasecmp HAVE_STRNCASECMP)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(strtoul HAVE_STRTOUL)
CHECK_FUNCTION_EXISTS(strcoll HAVE_STRCOLL)
CHECK_FUNCTION_EXISTS(strftime HAVE_STRFTIME)
CHECK_FUNCTION_EXISTS(vprintf HAVE_VPRINTF)
CHECK_TYPE_SIZE(ptrdiff_t SIZEOF_PTRDIFF_T)
IF(HAVE_SIZEOF_PTRDIFF_T)
	SET(HAVE_PTRDIFF_T 1)
ELSE()
	SET(HAVE_PTRDIFF_T 0)
ENDIF()
CHECK_TYPE_SIZE(size_t SIZEOF_SIZE_T)
IF(HAVE_SIZEOF_SIZE_T)
	SET(HAVE_SIZE_T 1)
ELSE()
	SET(HAVE_SIZE_T 0)
	SET(size_t "unsigned")
ENDIF()
CHECK_SYMBOL_EXISTS("struct tm" "sys/time.h" TM_IN_SYS_TIME)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/include/ta_config.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/include/ta_config.h")

#SET(priv_HEADERS
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_frame_priv.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_global.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_utility.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_def_ui.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames/ta_frame.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_magic_nb.h"
#	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_memory.h"
#)

SET(ta-lib_HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_abstract.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_common.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_defs.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_func.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_libc.h"
)

INCLUDE_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func"
)

SET(ta-lib_CSOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LINEARREG.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MAX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHAMMER.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLDRAGONFLYDOJI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLGRAVESTONEDOJI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLABANDONEDBABY.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MAVP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLCONCEALBABYSWALL.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_TRENDMODE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_IMI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLADVANCEBLOCK.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_AROONOSC.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLLONGLINE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MIDPOINT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_PPO.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_WMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_DCPHASE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSPINNINGTOP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ACCBANDS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLIDENTICAL3CROWS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LINEARREG_ANGLE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ADOSC.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3OUTSIDE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CORREL.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ULTOSC.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_AD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLUNIQUE3RIVER.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLRISEFALL3METHODS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLLADDERBOTTOM.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_PLUS_DM.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ADD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_STOCHRSI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHANGINGMAN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_NVI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_T3.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_PHASOR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLTASUKIGAP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_DEMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_RSI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MIN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSTICKSANDWICH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MINMAX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLENGULFING.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLDOJI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ACOS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLINNECK.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_STOCH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLPIERCING.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ASIN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MACD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_KAMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ATR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ADXR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLCLOSINGMARUBOZU.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLDARKCLOUDCOVER.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TRIX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_TRENDLINE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ROCP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLMORNINGSTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MIDPRICE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LOG10.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLGAPSIDESIDEWHITE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SIN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TSF.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_STOCHF.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CMO.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3STARSINSOUTH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_AROON.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TAN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SINH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_utility.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TYPPRICE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_NATR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3BLACKCROWS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_DCPERIOD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLBREAKAWAY.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_PLUS_DI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_STDDEV.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL2CROWS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MOM.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TRIMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TRANGE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHIKKAKE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLEVENINGDOJISTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MAXINDEX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_FLOOR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ROCR100.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSEPARATINGLINES.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHARAMICROSS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_WCLPRICE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_BBANDS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLLONGLEGGEDDOJI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CEIL.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_AVGDEV.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLCOUNTERATTACK.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_EXP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLONNECK.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLINVERTEDHAMMER.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLXSIDEGAP3METHODS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLMARUBOZU.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CCI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MACDEXT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLTHRUSTING.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLMATHOLD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHIKKAKEMOD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLKICKINGBYLENGTH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LINEARREG_INTERCEPT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLKICKING.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SUB.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LINEARREG_SLOPE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_HT_SINE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_COSH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLDOJISTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHOMINGPIGEON.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_EMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_COS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SAREXT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_LN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TANH.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MACDFIX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_OBV.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SUM.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_DX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_TEMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MININDEX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MULT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_APO.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_DIV.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_WILLR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSHORTLINE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLRICKSHAWMAN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MEDPRICE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLMATCHINGLOW.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ROCR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLBELTHOLD.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLTAKURI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLTRISTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHARAMI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_PVI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ATAN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLHIGHWAVE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSTALLEDPATTERN.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_VAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLEVENINGSTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MAMA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3LINESTRIKE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3WHITESOLDIERS.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLSHOOTINGSTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_BOP.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MINUS_DM.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ROC.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDLMORNINGDOJISTAR.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_AVGPRICE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MFI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MINMAXINDEX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_MINUS_DI.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_SQRT.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_BETA.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_CDL3INSIDE.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func/ta_ADX.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_version.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_retcode.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_global.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames/ta_frame.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_abstract.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_func_api.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_def_ui.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_group_idx.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_t.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_p.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_a.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_l.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_r.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_s.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_c.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_g.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_f.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_e.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_o.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_x.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_q.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_b.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_n.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_v.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_y.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_d.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_i.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_h.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_z.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_j.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_k.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_w.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_u.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_m.c"
)


ADD_LIBRARY(ta_lib SHARED ${ta-lib_CSOURCES})
SET_TARGET_PROPERTIES(ta_lib PROPERTIES
	SOVERSION ${TA_LIB_VERSION_FULL}
	DEFINE_SYMBOL TA_LIB_SHARED
	OUTPUT_NAME "ta_lib"
)
ADD_LIBRARY(ta_lib_static STATIC ${ta-lib_CSOURCES})

IF(UNIX)
	SET_TARGET_PROPERTIES(ta_lib_static PROPERTIES OUTPUT_NAME ta_lib)
ENDIF(UNIX)

IF(WIN32)
	# Note: WIN32 is defined regardless of 32 or 64 bits host.
	SET_TARGET_PROPERTIES(ta_lib_static PROPERTIES OUTPUT_NAME ta_lib_static)
ENDIF(WIN32)

IF(MSVC)
	add_compile_options(/deterministic)
ENDIF(MSVC)

TARGET_INCLUDE_DIRECTORIES(ta_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
TARGET_INCLUDE_DIRECTORIES(ta_lib_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
