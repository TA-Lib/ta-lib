
# gen_code should not be installed.
noinst_PROGRAMS = gen_code

gen_code_SOURCES = gen_code.c

gen_code_CPPFLAGS = -I../../ta_common
gen_code_LDFLAGS = -no-undefined
gen_code_LDADD = \
	../../ta_common/libta_common.la \
	../../ta_abstract/libta_abstract_gc.la \
	../../ta_func/libta_func.la \
	$(LIBM)

all-local: gen_code
	$(LIBTOOL) --mode=execute ../post-build-bin.sh gen_code
