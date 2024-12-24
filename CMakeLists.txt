cmake_minimum_required(VERSION 3.18)

project(ta-lib)

set(TA_LIB_VERSION_MAJOR 0)
set(TA_LIB_VERSION_MINOR 6)
set(TA_LIB_VERSION_PATCH 1)

set(PROJECT_VERSION "${TA_LIB_VERSION_MAJOR}.${TA_LIB_VERSION_MINOR}.${TA_LIB_VERSION_PATCH}")

set(CMAKE_C_STANDARD 11)

# Option to build development tools (enabled by default)
option(BUILD_DEV_TOOLS "Build development tools (gen_code, ta_regtest)" ON)
message(STATUS "BUILD_DEV_TOOLS: ${BUILD_DEV_TOOLS}")

# Default to Release config
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Attempt to set SOURCE_DATE_EPOCH to the last commit timestamp of include/ta_common.h
# If not possible, use a fallback. Controlling the SOURCE_DATE_EPOCH "might" help producing
# consistent binaries when there is no source code change.
function(set_fallback_source_date_epoch)
    # Generate a unique timestamp based on version numbers
    math(EXPR UNIQUE_TIMESTAMP "(${TA_LIB_VERSION_MAJOR} * 1000000) + (${TA_LIB_VERSION_MINOR} * 1000) + ${TA_LIB_VERSION_PATCH}")
    set(ENV{SOURCE_DATE_EPOCH} ${UNIQUE_TIMESTAMP})
	message(STATUS "SOURCE_DATE_EPOCH: $ENV{SOURCE_DATE_EPOCH} (${PROJECT_VERSION})")
endfunction()

if(NOT DEFINED ENV{SOURCE_DATE_EPOCH})
    # Check if the current directory is a Git repository
    execute_process(
        COMMAND git rev-parse --is-inside-work-tree
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE IS_GIT_REPO
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(IS_GIT_REPO AND IS_GIT_REPO STREQUAL "true")
        execute_process(
            COMMAND git log -1 --format=%ct -- include/ta_common.h
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_TIMESTAMP
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(GIT_COMMIT_TIMESTAMP)
            set(ENV{SOURCE_DATE_EPOCH} ${GIT_COMMIT_TIMESTAMP})
            message(STATUS "SOURCE_DATE_EPOCH: $ENV{SOURCE_DATE_EPOCH}")
        else()
            message(WARNING "include/ta_common.h not found. Using fallback SOURCE_DATE_EPOCH.")
            set_fallback_source_date_epoch()
        endif()
    else()
        message(WARNING "Not a Git repository. Using fallback SOURCE_DATE_EPOCH.")
        set_fallback_source_date_epoch()
    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
message(STATUS "CPACK_GENERATOR: ${CPACK_GENERATOR}")

# TA-Lib public API headers (will be installed by this script)
set(LIB_HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_abstract.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_common.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_defs.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_func.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/ta_libc.h"
)

# Source used in all end-user libraries and most TA-Lib executable.
set(COMMON_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_global.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_retcode.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common/ta_version.c"
)

# TA-Lib specify all its TA functions signature (and meta information
# such as the "group" it belongs to) using an "interface definition language"
# written in C.
set(IDL_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_abstract.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_def_ui.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_a.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_b.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_d.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_e.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_f.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_g.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_h.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_i.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_j.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_k.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_l.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_m.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_n.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_o.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_p.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_q.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_r.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_s.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_t.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_u.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_v.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_w.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_x.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_y.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/tables/table_z.c"
)


# Extract the file names from the LIB_HEADERS variable
set(LIB_HEADERS_FILENAMES)
foreach(header ${LIB_HEADERS})
	get_filename_component(header_name ${header} NAME)
    list(APPEND LIB_HEADERS_FILENAMES ${header_name})
endforeach()

###############################
# Shared and static libraries #
###############################
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckSymbolExists)
check_include_files(float.h HAVE_FLOAT_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(locale.h HAVE_LOCALE_H)
check_include_files(stddef.h HAVE_STDDEF_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(wchar.h HAVE_WCHAR_H)
check_include_files(wctype.h HAVE_WCTYPE_H)
check_function_exists(floor HAVE_FLOOR)
check_function_exists(isascii HAVE_ISASCII)
check_function_exists(localeconv HAVE_LOCALECONV)
check_function_exists(mblen HAVE_MBLEN)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(modf HAVE_MODF)
check_function_exists(pow HAVE_POW)
check_function_exists(sqrt HAVE_SQRT)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strchr HAVE_STRCHR)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(strrchr HAVE_STRRCHR)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtol HAVE_STRTOL)
check_function_exists(strtoul HAVE_STRTOUL)
check_function_exists(strcoll HAVE_STRCOLL)
check_function_exists(strftime HAVE_STRFTIME)
check_function_exists(vprintf HAVE_VPRINTF)
check_type_size(ptrdiff_t SIZEOF_PTRDIFF_T)
if(HAVE_SIZEOF_PTRDIFF_T)
	set(HAVE_PTRDIFF_T 1)
else()
	set(HAVE_PTRDIFF_T 0)
endif()
check_type_size(size_t SIZEOF_SIZE_T)
if(HAVE_SIZEOF_SIZE_T)
	set(HAVE_SIZE_T 1)
else()
	set(HAVE_SIZE_T 0)
	set(size_t "unsigned")
endif()
check_symbol_exists("struct tm" "sys/time.h" TM_IN_SYS_TIME)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/ta_config.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/include/ta_config.h")

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func"
)

set(LIB_SOURCES
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
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/frames/ta_frame.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_func_api.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_abstract/ta_group_idx.c"
)

list(APPEND LIB_SOURCES ${COMMON_SOURCES})
list(APPEND LIB_SOURCES ${IDL_SOURCES})

add_library(ta-lib SHARED ${LIB_SOURCES})
set_target_properties(ta-lib PROPERTIES
	SOVERSION ${PROJECT_VERSION}
	DEFINE_SYMBOL TA_LIB_SHARED
	OUTPUT_NAME "ta-lib"
)
add_library(ta-lib-static STATIC ${LIB_SOURCES})

if(UNIX)
	set_target_properties(ta-lib-static PROPERTIES OUTPUT_NAME ta-lib)
endif(UNIX)

if(WIN32)
	# Note: WIN32 is defined regardless of 32 or 64 bits host.
	set_target_properties(ta-lib-static PROPERTIES OUTPUT_NAME ta-lib-static)
endif(WIN32)

if(MSVC)
	add_compile_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:/deterministic>)
	add_compile_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>:/deterministic>)
    add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:/Brepro>)
    add_link_options($<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>:/Brepro>)
endif(MSVC)

target_include_directories(ta-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(ta-lib-static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Install the libraries and headers.
if(WIN32)
    install(TARGETS ta-lib ta-lib-static
        LIBRARY DESTINATION bin  # For shared libraries (DLLs on Windows)
        ARCHIVE DESTINATION lib  # For static libraries
    )
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
    )
else()
    install(TARGETS ta-lib ta-lib-static
        LIBRARY DESTINATION lib  # For shared libraries (.so on Linux)
        ARCHIVE DESTINATION lib  # For static libraries
    )
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION include/ta-lib
        FILES_MATCHING PATTERN "*.h"
    )
endif()

# Cleanup to avoid potential conflict with older installation:
#  - Remove deprecated "ta_lib" libs. New name is "ta-lib"
#  - Remove headers directly under /usr/include and /usr/local/include
#    Headers are now in ta-lib subdir (e.g. /usr/local/include/ta-lib/*)
#
# Also, if the user now installs under /usr/local, clean-up "conflicting"
# installation directly under /usr (and vice-versa).
#
if(UNIX)
	file(WRITE ${CMAKE_BINARY_DIR}/cleanup_glob.cmake
		"message(STATUS \"Running cleanup script with prefix [${CMAKE_INSTALL_PREFIX}]\")\n"
    	"file(GLOB OLD_LIB_FILES /usr/lib/*ta_lib* /usr/local/lib/*ta_lib*)\n"
    	"foreach(file \${OLD_LIB_FILES})\n"
    	"    file(REMOVE \${file})\n"
    	"endforeach()\n"
		"foreach(header ${LIB_HEADERS_FILENAMES})\n"
    	"    file(REMOVE /usr/local/include/\${header} /usr/include/\${header})\n"
    	"endforeach()\n"
		"if(\"${CMAKE_INSTALL_PREFIX}\" STREQUAL \"/usr\")\n"
        "    file(REMOVE_RECURSE /usr/local/include/ta-lib)\n"
        "    file(GLOB OLD_LIB_FILES /usr/local/lib/*ta-lib*)\n"
        "    foreach(file \${OLD_LIB_FILES})\n"
        "        file(REMOVE \${file})\n"
        "    endforeach()\n"
        "elseif(\"${CMAKE_INSTALL_PREFIX}\" STREQUAL \"/usr/local\")\n"
        "    file(REMOVE_RECURSE /usr/include/ta-lib)\n"
        "    file(GLOB OLD_LIB_FILES /usr/lib/*ta-lib*)\n"
        "    foreach(file \${OLD_LIB_FILES})\n"
        "        file(REMOVE \${file})\n"
        "    endforeach()\n"
        "endif()\n"
	)
	install(SCRIPT ${CMAKE_BINARY_DIR}/cleanup_glob.cmake )
endif()

if(BUILD_DEV_TOOLS)
	############
	# gen_code #
	############
	set(GEN_CODE_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/gen_code/gen_code.c"
	)

	list(APPEND GEN_CODE_SOURCES ${COMMON_SOURCES})
	list(APPEND GEN_CODE_SOURCES ${IDL_SOURCES})

	add_executable(gen_code ${GEN_CODE_SOURCES})

	target_compile_definitions(gen_code PRIVATE TA_GEN_CODE)

	set_target_properties(gen_code PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
	)

	install(TARGETS gen_code
		RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/bin"
	)

	# Add post-build to always check to copy gen_code to ta-lib/bin directory
	# This is to "save the day" for a user that would delete the bin content.
	add_custom_target(ensure_gen_code_in_bin ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:gen_code>"
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:gen_code>"
		DEPENDS gen_code
	)

	##############
	# ta_regtest #
	##############
	set(TA_REGTEST_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_regtest.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_candlestick.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_1in_1out.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_1in_2out.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/test_internals.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_adx.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_avgdev.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_bbands.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_imi.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_ma.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_macd.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_minmax.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_mom.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_per_ema.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_per_hl.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_per_hlc.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_per_hlcv.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_per_ohlc.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_po.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_rsi.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_sar.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_stddev.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_stoch.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/ta_test_func/test_trange.c"

		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/test_util.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/test_data.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/test_abstract.c"
	)

	add_executable(ta_regtest ${TA_REGTEST_SOURCES})

	# Set the include directories for ta_regtest
	target_include_directories(ta_regtest PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_common"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/ta_func"
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	)

	# Link the necessary libraries for ta_regtest (the 'm' is for the Math library)
	# On windows, no need for a math library. It is already included in the standard library.
	if(WIN32)
		target_link_libraries(ta_regtest PRIVATE ta-lib-static)
	else()
		target_link_libraries(ta_regtest PRIVATE ta-lib-static m)
	endif()

	# Set the output directory for the ta_regtest executable during the build process
	set_target_properties(ta_regtest PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
	)

	# Automatically install ta_regtest in ta-lib/bin after a successful build.
	install(TARGETS ta_regtest
		RUNTIME DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/bin"
	)

	# Add a custom target to ensure ta_regtest is copied to ta-lib/bin directory
	add_custom_target(ensure_ta_regtest_in_bin ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"$<TARGET_FILE:ta_regtest>"
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:ta_regtest>"
		DEPENDS ta_regtest
	)
endif()

#############
# Packaging #
#############

# Example of good assets naming:
#    https://cmake.org/download/

# Set variables controling the package creation.
set(CPACK_BUILD_CONFIG "${CMAKE_BUILD_TYPE}")

# Set common package metadata
set(CPACK_PACKAGE_NAME "ta-lib")
set(CPACK_PACKAGE_VERSION_MAJOR ${TA_LIB_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${TA_LIB_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${TA_LIB_VERSION_BUILD})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

set(CPACK_PACKAGE_VENDOR "ta-lib.org")
set(CPACK_PACKAGE_CONTACT "mfortier@yahoo.com")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TA-Lib Technical Analysis Library")
if(WIN32)
    # Use mix case for Windows "best practice" in "C:\Program Files"
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "TA-Lib")
else()
    # Keep everything lowercase for Unix-like systems
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ta-lib")
endif()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Detect host architecture and set DEB and RPM specific architecture variables
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    set(DEB_HOST_ARCH "amd64")
    set(RPM_HOST_ARCH "x86_64")
    set(MSI_HOST_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    set(DEB_HOST_ARCH "i386")
    set(RPM_HOST_ARCH "i386")
    set(MSI_HOST_ARCH "i386")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(DEB_HOST_ARCH "armhf")
    set(RPM_HOST_ARCH "armv7hl")
    set(MSI_HOST_ARCH "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(DEB_HOST_ARCH "arm64")
    set(RPM_HOST_ARCH "aarch64")
    set(MSI_HOST_ARCH "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64le")
    set(DEB_HOST_ARCH "ppc64el")
    set(RPM_HOST_ARCH "ppc64le")
    set(MSI_HOST_ARCH "ppc64le")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "s390x")
    set(DEB_HOST_ARCH "s390x")
    set(RPM_HOST_ARCH "s390x")
    set(MSI_HOST_ARCH "s390x")
else()
    set(DEB_HOST_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    set(RPM_HOST_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    set(MSI_HOST_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Debian package created by TA-LIb CI only if host is ubuntu.
# The package should work on any debian-based Linux.
#
# More info: https://www.debian.org/doc/debian-policy/ch-controlfields.html#source
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${DEB_HOST_ARCH}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "mfortier@yahoo.com")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")

# RPM package created only if host is red-hat based.
set(CPACK_RPM_PACKAGE_ARCHITECTURE "${RPM_HOST_ARCH}")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_LICENSE "BSD")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc")

# MSI package created only if host is windows.

# CPACK_WIX_VERSION "4" is for ""4 and later" and is the one to use for Wix 5
# References:
#    https://cmake.org/cmake/help/latest/cpack_gen/wix.html#variable:CPACK_WIX_VERSION
#    https://stackoverflow.com/questions/77584815/how-to-use-wix-v4-within-cmake-to-build-an-installation-package-for-windows
set(CPACK_WIX_VERSION "4")
set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.rtf")
#set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/banner.bmp")
#set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/dialog.bmp")

# Set a constant UPGRADE_GUID (never change this)
set(CPACK_WIX_UPGRADE_GUID "37b08339-521a-422f-a964-e616bd3e06b9")

# Set the PRODUCT_GUID to change on every version change.
#
# This should guarantee a single version of the library installed
# (older ones always uninstalled first).
string(MD5 PRODUCT_GUID_HASH "${CPACK_WIX_UPGRADE_GUID}${CPACK_PACKAGE_VERSION}")
string(SUBSTRING ${PRODUCT_GUID_HASH} 0 8 PRODUCT_GUID_PART1)
string(SUBSTRING ${PRODUCT_GUID_HASH} 8 4 PRODUCT_GUID_PART2)
string(SUBSTRING ${PRODUCT_GUID_HASH} 12 4 PRODUCT_GUID_PART3)
string(SUBSTRING ${PRODUCT_GUID_HASH} 16 4 PRODUCT_GUID_PART4)
string(SUBSTRING ${PRODUCT_GUID_HASH} 20 12 PRODUCT_GUID_PART5)
set(CPACK_WIX_PRODUCT_GUID "${PRODUCT_GUID_PART1}-${PRODUCT_GUID_PART2}-${PRODUCT_GUID_PART3}-${PRODUCT_GUID_PART4}-${PRODUCT_GUID_PART5}")

# Set common variables that changes depending of the generator.
if(CPACK_GENERATOR STREQUAL "DEB")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${DEB_HOST_ARCH}")
elseif(CPACK_GENERATOR STREQUAL "RPM")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${RPM_HOST_ARCH}")
elseif(CPACK_GENERATOR STREQUAL "WIX")
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${MSI_HOST_ARCH}")
endif()

# Allow the caller at the "cmake" configuration phase to see the filename.
message(STATUS "CPACK_PACKAGE_FILE_NAME: ${CPACK_PACKAGE_FILE_NAME}")

include(CPack)
