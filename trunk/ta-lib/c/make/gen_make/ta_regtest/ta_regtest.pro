TEMPLATE  = app
CONFIG   -= qt

# Force this application to be a console application
CONFIG   -= windows
CONFIG   += console

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd
cmr:OBJECTS_DIR = ../../../../../temp/cmr
csd:OBJECTS_DIR = ../../../../../temp/csd
csr:OBJECTS_DIR = ../../../../../temp/csr
cdr:OBJECTS_DIR = ../../../../../temp/cdr
cdd:OBJECTS_DIR = ../../../../../temp/cdd

# Output info
TARGET      = ta_regtest
DESTDIR     = ../../../../../bin

# Files to process
SOURCES	= ../../../../../src/tools/ta_regtest/ta_regtest.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_1in_1out.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_1in_2out.c \
          ../../../../../src/tools/ta_regtest/test_ascii.c \
          ../../../../../src/tools/ta_regtest/test_internals.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_adx.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_bbands.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_ma.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_macd.c \
          ../../../../../src/tools/ta_regtest/test_merge.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_minmax.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_mom.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_per_ema.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_per_hl.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_per_hlc.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_per_hlcv.c \
          ../../../../../src/tools/ta_regtest/test_period.c \
          ../../../../../src/tools/ta_regtest/test_pm.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_po.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_rsi.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_sar.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_stddev.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_stoch.c \
          ../../../../../src/tools/ta_regtest/ta_test_func/test_trange.c \
          ../../../../../src/tools/ta_regtest/test_util.c \
          ../../../../../src/tools/ta_regtest/test_abstract.c \
          ../../../../../src/tools/ta_regtest/test_csi.c \
          ../../../../../src/tools/ta_regtest/test_yahoo.c

# Additional libraries
win32:TA_LIBC_CMD = ta_libc_cmd.lib
win32:TA_LIBC_CMR = ta_libc_cmr.lib
win32:TA_LIBC_CSD = ta_libc_csd.lib
win32:TA_LIBC_CSR = ta_libc_csr.lib
win32:TA_LIBC_CDR = ta_libc_cdr.lib
win32:TA_LIBC_CDD = ta_libc_cdd.lib

unix:TA_LIBC_CMD  = libta_libc_cmd.a
unix:TA_LIBC_CMR  = libta_libc_cmr.a
unix:TA_LIBC_CSD  = libta_libc_csd.a
unix:TA_LIBC_CSR  = libta_libc_csr.a
unix:TA_LIBC_CDR  = libta_libc_cdr.a
unix:TA_LIBC_CDD  = libta_libc_cdd.a

cmd:LIBS += ../../../../../lib/$$TA_LIBC_CMD
cmr:LIBS += ../../../../../lib/$$TA_LIBC_CMR
csd:LIBS += ../../../../../lib/$$TA_LIBC_CSD
csr:LIBS += ../../../../../lib/$$TA_LIBC_CSR
cdr:LIBS += ../../../../../lib/$$TA_LIBC_CDR
cdd:LIBS += ../../../../../lib/$$TA_LIBC_CDD

win32:LIBS += wininet.lib odbc32.lib
win32-msvc:LIBS += odbccp32.lib
unix:LIBS += -lcurl -lssl -lcrypto -ldl

# Compiler Options
INCLUDEPATH = ../../../../../src/ta_common \
              ../../../../../src/tools/ta_regtest \
              ../../../../../src/ta_func \
              ../../../../../include \
              ../../../../../src/ta_common/imatix/sfl \
              ../../../../../src/ta_common/trio

# debug/release dependent options.
debug:DEFINES   *= TA_DEBUG
debug:DEFINES   *= _DEBUG
DEFINES        += TA_SINGLE_THREAD
thread:DEFINES -= TA_SINGLE_THREAD


# Platform dependent options.
win32:DEFINES         *= WIN32
win32-msvc:DEFINES    *= _MBCS _LIB
cygwin-g++:LIBS       -= -ldl

cmd:TEMP_CLEAN_ALL = ../../../../../temp/cmd/*.pch
cmr:TEMP_CLEAN_ALL = ../../../../../temp/cmr/*.pch
csd:TEMP_CLEAN_ALL = ../../../../../temp/csd/*.pch
csr:TEMP_CLEAN_ALL = ../../../../../temp/csr/*.pch
cdr:TEMP_CLEAN_ALL = ../../../../../temp/cdr/*.pch
cdd:TEMP_CLEAN_ALL = ../../../../../temp/cdd/*.pch

cmd:TEMP_CLEAN_ALL2 = ../../../../../temp/cmd/*.idb
cmr:TEMP_CLEAN_ALL2 = ../../../../../temp/cmr/*.idb
csd:TEMP_CLEAN_ALL2 = ../../../../../temp/csd/*.idb
csr:TEMP_CLEAN_ALL2 = ../../../../../temp/csr/*.idb
cdr:TEMP_CLEAN_ALL2 = ../../../../../temp/cdr/*.idb
cdd:TEMP_CLEAN_ALL2 = ../../../../../temp/cdd/*.idb

cmd:TEMP_CLEAN_ALL3 = ../../../../../temp/cmd/$$TARGET/*.pch
cmr:TEMP_CLEAN_ALL3 = ../../../../../temp/cmr/$$TARGET/*.pch
csd:TEMP_CLEAN_ALL3 = ../../../../../temp/csd/$$TARGET/*.pch
csr:TEMP_CLEAN_ALL3 = ../../../../../temp/csr/$$TARGET/*.pch
cdr:TEMP_CLEAN_ALL3 = ../../../../../temp/cdr/$$TARGET/*.pch
cdd:TEMP_CLEAN_ALL3 = ../../../../../temp/cdd/$$TARGET/*.pch

cmd:TEMP_CLEAN_ALL4 = ../../../../../temp/cmd/$$TARGET/*.idb
cmr:TEMP_CLEAN_ALL4 = ../../../../../temp/cmr/$$TARGET/*.idb
csd:TEMP_CLEAN_ALL4 = ../../../../../temp/csd/$$TARGET/*.idb
csr:TEMP_CLEAN_ALL4 = ../../../../../temp/csr/$$TARGET/*.idb
cdr:TEMP_CLEAN_ALL4 = ../../../../../temp/cdr/$$TARGET/*.idb
cdd:TEMP_CLEAN_ALL4 = ../../../../../temp/cdd/$$TARGET/*.idb

cmd:TEMP_CLEAN_ALL5 = ../../../../../temp/cmd/$$TARGET/*.obj
cmr:TEMP_CLEAN_ALL5 = ../../../../../temp/cmr/$$TARGET/*.obj
csd:TEMP_CLEAN_ALL5 = ../../../../../temp/csd/$$TARGET/*.obj
csr:TEMP_CLEAN_ALL5 = ../../../../../temp/csr/$$TARGET/*.obj
cdr:TEMP_CLEAN_ALL5 = ../../../../../temp/cdr/$$TARGET/*.obj
cdd:TEMP_CLEAN_ALL5 = ../../../../../temp/cdd/$$TARGET/*.obj

win32:CLEAN_FILES = ../../../../../bin/*.map ../../../../../bin/*._xe ../../../../../bin/*.tds ../../../../../bin/*.pdb ../../../../../bin/*.pbo ../../../../../bin/*.pbi ../../../../../bin/*.pbt $$TEMP_CLEAN_ALL $$TEMP_CLEAN_ALL2 $$TEMP_CLEAN_ALL3 $$TEMP_CLEAN_ALL4 $$TEMP_CLEAN_ALL5
