TEMPLATE  = lib
CONFIG   -= qt
CONFIG   += staticlib

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd
cmr:OBJECTS_DIR = ../../../../../temp/cmr
cmp:OBJECTS_DIR = ../../../../../temp/cmp
csd:OBJECTS_DIR = ../../../../../temp/csd
csr:OBJECTS_DIR = ../../../../../temp/csr
csp:OBJECTS_DIR = ../../../../../temp/csp
cdr:OBJECTS_DIR = ../../../../../temp/cdr
cdd:OBJECTS_DIR = ../../../../../temp/cdd

# Identify the target
LIBTARGET = ta_data
cmd:TARGET = ta_data_cmd
cmr:TARGET = ta_data_cmr
cmp:TARGET = ta_data_cmp
csd:TARGET = ta_data_csd
csr:TARGET = ta_data_csr
csp:TARGET = ta_data_csp
cdr:TARGET = ta_data_cdr
cdd:TARGET = ta_data_cdd

# Output info
DESTDIR     = ../../../../../lib

# Files to process
SOURCES	= ../../../../../src/ta_data/ta_source/ta_ascii/ta_ascii.c \
          ../../../../../src/ta_data/ta_source/ta_ascii/ta_ascii_handle.c \
          ../../../../../src/ta_data/ta_source/ta_fileindex/ta_build_index.c \
          ../../../../../src/ta_data/ta_data_interface.c \
          ../../../../../src/ta_data/ta_source/ta_fileindex/ta_fileindex.c \
          ../../../../../src/ta_data/ta_source/ta_fileindex/ta_fileindex_priv.c \
          ../../../../../src/ta_data/ta_source/ta_fileindex/ta_fileindex_utils.c \
          ../../../../../src/ta_data/ta_history/ta_history_builder.c \
          ../../../../../src/ta_data/ta_history/ta_historycheck.c \
          ../../../../../src/ta_data/ta_source/ta_fileindex/ta_parse_path.c \
          ../../../../../src/ta_data/ta_history/ta_period.c \
          ../../../../../src/ta_data/ta_source/ta_readop/ta_readop.c \
          ../../../../../src/ta_data/ta_source/ta_readop/ta_readop_do.c \
          ../../../../../src/ta_data/ta_source/ta_readop/ta_readop_estalloc.c \
          ../../../../../src/ta_data/ta_source/ta_readop/ta_readop_optimize.c \
          ../../../../../src/ta_data/ta_report.c \
          ../../../../../src/ta_data/ta_network.c \
          ../../../../../src/ta_data/ta_webfetch.c \
          ../../../../../src/ta_data/ta_source/ta_simulator/ta_simulator.c \
          ../../../../../src/ta_data/ta_source/ta_source.c \
          ../../../../../src/ta_data/ta_source/ta_sql/ta_sql.c \
          ../../../../../src/ta_data/ta_source/ta_sql/ta_sql_handle.c \
          ../../../../../src/ta_data/ta_source/ta_sql/ta_sql_local.c \
          ../../../../../src/ta_data/ta_source/ta_sql/ta_sql_minidriver.c \
          ../../../../../src/ta_data/ta_token.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_handle.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_historical.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_market.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_idx.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_one_symbol.c \
          ../../../../../src/ta_data/ta_source/ta_csi/ta_csi.c \
          ../../../../../src/ta_data/ta_source/ta_csi/ta_csi_handle.c \
          ../../../../../src/ta_data/ta_source/ta_csi/ta_csi_files.c \
          ../../../../../src/tools/gen_data/ta_daily_ref_0.c \
          ../../../../../src/tools/gen_data/ta_intra_ref_0.c \          
          ../../../../../src/tools/gen_data/ta_mrg_0.c

# Files to include (depending of platform)
win32:SOURCES *= ../../../../../src/ta_data/ta_source/ta_sql/ta_sql_odbc.c

# Compiler Options
INCLUDEPATH = ../../../../../src/ta_common \
              ../../../../../src/ta_data \
              ../../../../../src/ta_data/ta_source \
              ../../../../../src/ta_data/ta_source/ta_fileindex \
              ../../../../../src/ta_data/ta_source/ta_ascii \
              ../../../../../src/ta_data/ta_source/ta_readop \
              ../../../../../src/ta_data/ta_source/ta_yahoo \
              ../../../../../src/ta_data/ta_source/ta_sql \
              ../../../../../src/ta_data/ta_source/ta_csi \
              ../../../../../src/ta_data/ta_history \
              ../../../../../src/ta_data/ta_source/ta_simulator \
              ../../../../../src/ta_common/imatix/sfl \
              ../../../../../src/ta_common/kazlib \
              ../../../../../include

# debug/release dependent options.
debug:DEFINES   *= TA_DEBUG
debug:DEFINES   *= _DEBUG
DEFINES        += TA_SINGLE_THREAD
thread:DEFINES -= TA_SINGLE_THREAD


# Platform dependent options.
win32:DEFINES         *= WIN32
win32-msvc:DEFINES    *= _MBCS _LIB
freebsd-g++:LIBS      -= -ldl
freebsd-g++:INCLUDEPATH += /usr/local/include

cmd:TEMP_CLEAN_ALL = ../../../../../temp/cmd/*.pch
cmr:TEMP_CLEAN_ALL = ../../../../../temp/cmr/*.pch
cmp:TEMP_CLEAN_ALL = ../../../../../temp/cmp/*.pch
csd:TEMP_CLEAN_ALL = ../../../../../temp/csd/*.pch
csr:TEMP_CLEAN_ALL = ../../../../../temp/csr/*.pch
csp:TEMP_CLEAN_ALL = ../../../../../temp/csp/*.pch
cdr:TEMP_CLEAN_ALL = ../../../../../temp/cdr/*.pch
cdd:TEMP_CLEAN_ALL = ../../../../../temp/cdd/*.pch

cmd:TEMP_CLEAN_ALL2 = ../../../../../temp/cmd/*.idb
cmr:TEMP_CLEAN_ALL2 = ../../../../../temp/cmr/*.idb
cmp:TEMP_CLEAN_ALL2 = ../../../../../temp/cmp/*.idb
csd:TEMP_CLEAN_ALL2 = ../../../../../temp/csd/*.idb
csr:TEMP_CLEAN_ALL2 = ../../../../../temp/csr/*.idb
csp:TEMP_CLEAN_ALL2 = ../../../../../temp/csp/*.idb
cdr:TEMP_CLEAN_ALL2 = ../../../../../temp/cdr/*.idb
cdd:TEMP_CLEAN_ALL2 = ../../../../../temp/cdd/*.idb

cmd:TEMP_CLEAN_ALL3 = ../../../../../temp/cmd/$$TARGET/*.pch
cmr:TEMP_CLEAN_ALL3 = ../../../../../temp/cmr/$$TARGET/*.pch
cmp:TEMP_CLEAN_ALL3 = ../../../../../temp/cmp/$$TARGET/*.pch
csd:TEMP_CLEAN_ALL3 = ../../../../../temp/csd/$$TARGET/*.pch
csr:TEMP_CLEAN_ALL3 = ../../../../../temp/csr/$$TARGET/*.pch
csp:TEMP_CLEAN_ALL3 = ../../../../../temp/csp/$$TARGET/*.pch
cdr:TEMP_CLEAN_ALL3 = ../../../../../temp/cdr/$$TARGET/*.pch
cdd:TEMP_CLEAN_ALL3 = ../../../../../temp/cdd/$$TARGET/*.pch

cmd:TEMP_CLEAN_ALL4 = ../../../../../temp/cmd/$$TARGET/*.idb
cmr:TEMP_CLEAN_ALL4 = ../../../../../temp/cmr/$$TARGET/*.idb
cmp:TEMP_CLEAN_ALL4 = ../../../../../temp/cmp/$$TARGET/*.idb
csd:TEMP_CLEAN_ALL4 = ../../../../../temp/csd/$$TARGET/*.idb
csr:TEMP_CLEAN_ALL4 = ../../../../../temp/csr/$$TARGET/*.idb
csp:TEMP_CLEAN_ALL4 = ../../../../../temp/csp/$$TARGET/*.idb
cdr:TEMP_CLEAN_ALL4 = ../../../../../temp/cdr/$$TARGET/*.idb
cdd:TEMP_CLEAN_ALL4 = ../../../../../temp/cdd/$$TARGET/*.idb

cmd:TEMP_CLEAN_ALL5 = ../../../../../temp/cmd/$$TARGET/*.obj
cmr:TEMP_CLEAN_ALL5 = ../../../../../temp/cmr/$$TARGET/*.obj
cmp:TEMP_CLEAN_ALL5 = ../../../../../temp/cmp/$$TARGET/*.obj
csd:TEMP_CLEAN_ALL5 = ../../../../../temp/csd/$$TARGET/*.obj
csr:TEMP_CLEAN_ALL5 = ../../../../../temp/csr/$$TARGET/*.obj
csp:TEMP_CLEAN_ALL5 = ../../../../../temp/csp/$$TARGET/*.obj
cdr:TEMP_CLEAN_ALL5 = ../../../../../temp/cdr/$$TARGET/*.obj
cdd:TEMP_CLEAN_ALL5 = ../../../../../temp/cdd/$$TARGET/*.obj

win32:CLEAN_FILES = ../../../../../bin/*.map ../../../../../bin/*._xe ../../../../../bin/*.tds ../../../../../bin/*.pdb ../../../../../bin/*.pbo ../../../../../bin/*.pbi ../../../../../bin/*.pbt $$TEMP_CLEAN_ALL $$TEMP_CLEAN_ALL2 $$TEMP_CLEAN_ALL3 $$TEMP_CLEAN_ALL4 $$TEMP_CLEAN_ALL5