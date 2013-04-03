
#SET(TA_LIB_FLAGS "")
#SET(TA_LIB_INCLUDE_DIRS "")
#SET(TA_LIB_LIBRARY_DIRS "")
#SET(TA_LIB_LIBRARIES "")

SET(LIBRARY_OUTPUT_PATH ${ta-lib_BINARY_DIR}/lib/ CACHE PATH "Single out dir for all libs" FORCE)
SET(TA_LIB_LIBRARY_DIRS lib)
LINK_DIRECTORIES(${ta-lib_BINARY_DIR}/lib)

#SET(priv_HEADERS
#	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_frame_priv.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_global.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_utility.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_def_ui.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/frames/ta_frame.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_magic_nb.h
#	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_memory.h
#)

SET(ta-lib_HEADERS
	${ta-lib_SOURCE_DIR}/c/include/ta_abstract.h
	${ta-lib_SOURCE_DIR}/c/include/ta_common.h
	${ta-lib_SOURCE_DIR}/c/include/ta_defs.h
	${ta-lib_SOURCE_DIR}/c/include/ta_func.h
	${ta-lib_SOURCE_DIR}/c/include/ta_libc.h
)

INCLUDE_DIRECTORIES(${ta-lib_SOURCE_DIR}/c/include
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract 
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/frames
	${ta-lib_SOURCE_DIR}/c/src/ta_common
	${ta-lib_SOURCE_DIR}/c/src/ta_func
)

SET(ta-lib_CSOURCES
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LINEARREG.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MAX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHAMMER.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLDRAGONFLYDOJI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLGRAVESTONEDOJI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLABANDONEDBABY.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MAVP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLCONCEALBABYSWALL.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_TRENDMODE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_IMI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLADVANCEBLOCK.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_AROONOSC.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLLONGLINE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MIDPOINT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_PPO.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_WMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_DCPHASE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSPINNINGTOP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ACCBANDS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLIDENTICAL3CROWS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LINEARREG_ANGLE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ADOSC.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3OUTSIDE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CORREL.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ULTOSC.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_AD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLUNIQUE3RIVER.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLRISEFALL3METHODS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLLADDERBOTTOM.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_PLUS_DM.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ADD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_STOCHRSI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHANGINGMAN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_NVI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_T3.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_PHASOR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLTASUKIGAP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_DEMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_RSI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MIN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSTICKSANDWICH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MINMAX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLENGULFING.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLDOJI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ACOS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLINNECK.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_STOCH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLPIERCING.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ASIN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MACD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_KAMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ATR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ADXR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLCLOSINGMARUBOZU.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLDARKCLOUDCOVER.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TRIX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_TRENDLINE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ROCP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLMORNINGSTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MIDPRICE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LOG10.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLGAPSIDESIDEWHITE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SIN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TSF.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_STOCHF.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CMO.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3STARSINSOUTH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_AROON.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TAN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SINH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_utility.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TYPPRICE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_NATR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3BLACKCROWS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_DCPERIOD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLBREAKAWAY.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_PLUS_DI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_STDDEV.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL2CROWS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MOM.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TRIMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TRANGE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHIKKAKE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLEVENINGDOJISTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MAXINDEX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_FLOOR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ROCR100.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSEPARATINGLINES.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHARAMICROSS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_WCLPRICE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_BBANDS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLLONGLEGGEDDOJI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CEIL.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_AVGDEV.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLCOUNTERATTACK.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_EXP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLONNECK.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLINVERTEDHAMMER.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLXSIDEGAP3METHODS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLMARUBOZU.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CCI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MACDEXT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLTHRUSTING.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLMATHOLD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHIKKAKEMOD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLKICKINGBYLENGTH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LINEARREG_INTERCEPT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLKICKING.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SUB.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LINEARREG_SLOPE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_HT_SINE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_COSH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLDOJISTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHOMINGPIGEON.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_EMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_COS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SAREXT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_LN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TANH.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MACDFIX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_OBV.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SUM.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_DX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_TEMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MININDEX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MULT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_APO.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_DIV.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_WILLR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSHORTLINE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLRICKSHAWMAN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MEDPRICE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLMATCHINGLOW.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ROCR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLBELTHOLD.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLTAKURI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLTRISTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHARAMI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_PVI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ATAN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLHIGHWAVE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSTALLEDPATTERN.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_VAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLEVENINGSTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MAMA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3LINESTRIKE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3WHITESOLDIERS.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLSHOOTINGSTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_BOP.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MINUS_DM.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ROC.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDLMORNINGDOJISTAR.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_AVGPRICE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MFI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MINMAXINDEX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_MINUS_DI.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_SQRT.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_BETA.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_CDL3INSIDE.c
	${ta-lib_SOURCE_DIR}/c/src/ta_func/ta_ADX.c
	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_version.c
	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_retcode.c
	${ta-lib_SOURCE_DIR}/c/src/ta_common/ta_global.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/frames/ta_frame.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_abstract.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_func_api.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_def_ui.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/ta_group_idx.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_t.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_p.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_a.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_l.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_r.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_s.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_c.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_g.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_f.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_e.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_o.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_x.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_q.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_b.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_n.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_v.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_y.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_d.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_i.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_h.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_z.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_j.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_k.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_w.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_u.c
	${ta-lib_SOURCE_DIR}/c/src/ta_abstract/tables/table_m.c
)

IF(UNIX)
	ADD_LIBRARY(ta_lib SHARED ${ta-lib_CSOURCES})
	SET_TARGET_PROPERTIES(ta_lib PROPERTIES SOVERSION ${TA_LIB_VERSION_FULL} DEFINE_SYMBOL TA_LIB_SHARED)
	ADD_LIBRARY(ta_lib_static STATIC ${ta-lib_CSOURCES})
	SET_TARGET_PROPERTIES(ta_lib_static PROPERTIES OUTPUT_NAME ta_lib)
ENDIF(UNIX)

IF(WIN32)
	INCLUDE(GenerateExportHeader)
	ADD_LIBRARY(ta_lib SHARED ${ta-lib_CSOURCES})
	SET_TARGET_PROPERTIES(ta_lib PROPERTIES DEFINE_SYMBOL TA_LIB_SHARED)
	ADD_LIBRARY(ta_lib_static STATIC ${ta-lib_CSOURCES})
	SET_TARGET_PROPERTIES(ta_lib_static PROPERTIES OUTPUT_NAME ta_lib_a)
ENDIF(WIN32)

SET_TARGET_PROPERTIES(ta_lib PROPERTIES SOVERSION ${TA_LIB_VERSION_FULL})

INSTALL(FILES ${ta-lib_HEADERS} DESTINATION include/ta-lib)
INSTALL(TARGETS ta_lib ta_lib_static DESTINATION lib)

