TEMPLATE  = lib
CONFIG   -= qt
CONFIG   += staticlib

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd
cmr:OBJECTS_DIR = ../../../../../temp/cmr
csd:OBJECTS_DIR = ../../../../../temp/csd
csr:OBJECTS_DIR = ../../../../../temp/csr

# Identify the target
LIBTARGET = ta_data
cmd:TARGET = ta_data_cmd
cmr:TARGET = ta_data_cmr
csd:TARGET = ta_data_csd
csr:TARGET = ta_data_csr

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
          ../../../../../src/ta_data/ta_token.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_handle.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_historical.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_market.c \
          ../../../../../src/ta_data/ta_source/ta_yahoo/ta_yahoo_idx.c \
          ../../../../../src/tools/gen_data/ta_daily_ref_0.c \
          ../../../../../src/tools/gen_data/ta_intra_ref_0.c \          
          ../../../../../src/tools/gen_data/ta_mrg_0.c



# Compiler Options
INCLUDEPATH = ../../../../../src/ta_common \
              ../../../../../src/ta_data \
              ../../../../../src/ta_data/ta_source \
              ../../../../../src/ta_data/ta_source/ta_fileindex \
              ../../../../../src/ta_data/ta_source/ta_ascii \
              ../../../../../src/ta_data/ta_source/ta_readop \
              ../../../../../src/ta_data/ta_source/ta_yahoo \
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

cmd:TEMP_CLEAN_ALL = ../../../../../temp/cmd/*.pch
cmr:TEMP_CLEAN_ALL = ../../../../../temp/cmr/*.pch
csd:TEMP_CLEAN_ALL = ../../../../../temp/csd/*.pch
csr:TEMP_CLEAN_ALL = ../../../../../temp/csr/*.pch

cmd:TEMP_CLEAN_ALL2 = ../../../../../temp/cmd/*.idb
cmr:TEMP_CLEAN_ALL2 = ../../../../../temp/cmr/*.idb
csd:TEMP_CLEAN_ALL2 = ../../../../../temp/csd/*.idb
csr:TEMP_CLEAN_ALL2 = ../../../../../temp/csr/*.idb

cmd:TEMP_CLEAN_ALL3 = ../../../../../temp/cmd/$$TARGET/*.pch
cmr:TEMP_CLEAN_ALL3 = ../../../../../temp/cmr/$$TARGET/*.pch
csd:TEMP_CLEAN_ALL3 = ../../../../../temp/csd/$$TARGET/*.pch
csr:TEMP_CLEAN_ALL3 = ../../../../../temp/csr/$$TARGET/*.pch

cmd:TEMP_CLEAN_ALL4 = ../../../../../temp/cmd/$$TARGET/*.idb
cmr:TEMP_CLEAN_ALL4 = ../../../../../temp/cmr/$$TARGET/*.idb
csd:TEMP_CLEAN_ALL4 = ../../../../../temp/csd/$$TARGET/*.idb
csr:TEMP_CLEAN_ALL4 = ../../../../../temp/csr/$$TARGET/*.idb

cmd:TEMP_CLEAN_ALL5 = ../../../../../temp/cmd/$$TARGET/*.obj
cmr:TEMP_CLEAN_ALL5 = ../../../../../temp/cmr/$$TARGET/*.obj
csd:TEMP_CLEAN_ALL5 = ../../../../../temp/csd/$$TARGET/*.obj
csr:TEMP_CLEAN_ALL5 = ../../../../../temp/csr/$$TARGET/*.obj

win32:CLEAN_FILES = ../../../../../bin/*.map ../../../../../bin/*._xe ../../../../../bin/*.tds ../../../../../bin/*.pdb ../../../../../bin/*.pbo ../../../../../bin/*.pbi ../../../../../bin/*.pbt $$TEMP_CLEAN_ALL $$TEMP_CLEAN_ALL2 $$TEMP_CLEAN_ALL3 $$TEMP_CLEAN_ALL4 $$TEMP_CLEAN_ALL5