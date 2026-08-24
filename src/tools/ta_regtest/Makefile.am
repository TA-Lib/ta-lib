
# ta_regrest is used for testing and should
# not be installed.
noinst_PROGRAMS = ta_regtest

ta_regtest_SOURCES = ta_regtest.c \
	test_data.c \
	test_util.c \
	test_abstract.c \
	codegen_pipe.c \
	test_codegen.c \
	server_verify.c \
	ta_test_reference.c \
	ta_test_reference_golden.c \
	ta_test_func/test_adx.c \
	ta_test_func/test_mom.c \
	ta_test_func/test_sar.c \
	ta_test_func/test_rsi.c \
	ta_test_func/test_candlestick.c \
	ta_test_func/test_per_ema.c \
	ta_test_func/test_per_hlc.c \
	ta_test_func/test_stoch.c \
	ta_test_func/test_macd.c \
	ta_test_func/test_minmax.c \
	ta_test_func/test_rolling_extremum.c \
	ta_test_func/test_per_hlcv.c \
	ta_test_func/test_per_cv.c \
	ta_test_func/test_1in_1out.c \
	ta_test_func/test_1in_2out.c \
	ta_test_func/test_per_ohlc.c \
	ta_test_func/test_stddev.c \
	ta_test_func/test_correl.c \
	ta_test_func/test_beta.c \
	ta_test_func/test_linearreg.c \
	ta_test_func/test_reference.c \
	ta_test_func/test_bbands.c \
	ta_test_func/test_ma.c \
	ta_test_func/test_po.c \
	ta_test_func/test_per_hl.c \
	ta_test_func/test_trange.c \
	ta_test_func/test_imi.c \
	ta_test_func/test_avgdev.c \
	ta_test_func/test_period_boundary.c \
	ta_test_func/test_mavp.c \
	ta_test_func/test_s_overflow.c \
	ta_test_func/test_stream_finite.c \
	ta_test_func/test_composite1.c \
	ta_test_func/test_composite2.c \
	ta_test_func/test_marketfi.c \
	ta_test_func/test_vwap.c \
	ta_test_func/test_cmf.c \
	ta_test_func/test_mfi.c \
	ta_test_func/test_cmou.c \
	ta_test_func/test_variants.c \
	ta_test_legacy.c \
	test_internals.c

ta_regtest_CPPFLAGS = -I../../ta_func \
		      -I../../ta_common/trio \
		      -I../../ta_common/mt \
		      -I../../ta_common \
		      -I../../ta_abstract
ta_regtest_LDFLAGS = -no-undefined
ta_regtest_LDADD = ../../libta-lib.la $(LIBM)

all-local: ta_regtest
	$(LIBTOOL) --mode=execute ../post-build-bin.sh ta_regtest
