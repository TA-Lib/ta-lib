TEMPLATE   = lib
CONFIG    -= qt
CONFIG    += staticlib

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd
cmr:OBJECTS_DIR = ../../../../../temp/cmr
csd:OBJECTS_DIR = ../../../../../temp/csd
csr:OBJECTS_DIR = ../../../../../temp/csr
cdr:OBJECTS_DIR = ../../../../../temp/cdr
cdd:OBJECTS_DIR = ../../../../../temp/cdd

# Identify the target name
LIBTARGET = ta_common
cmd:TARGET = ta_common_cmd
cmr:TARGET = ta_common_cmr
csd:TARGET = ta_common_csd
csr:TARGET = ta_common_csr
cdr:TARGET = ta_common_cdr
cdd:TARGET = ta_common_cdd

# Output info
DESTDIR     = ../../../../../lib

# Files to process
SOURCES	= ../../../../../src/ta_common/bzip2/blocksort.c \
          ../../../../../src/ta_common/bzip2/bzlib.c \
          ../../../../../src/ta_common/bzip2/compress.c \
          ../../../../../src/ta_common/bzip2/crctable.c \
          ../../../../../src/ta_common/bzip2/decompress.c \
          ../../../../../src/ta_common/bzip2/huffman.c \
          ../../../../../src/ta_common/bzip2/randtable.c \
          ../../../../../src/ta_common/kazlib/dict.c \
          ../../../../../src/ta_common/kazlib/list.c \
          ../../../../../src/ta_common/imatix/sfl/sflcryp.c \
          ../../../../../src/ta_common/imatix/sfl/sfldate.c \
          ../../../../../src/ta_common/imatix/sfl/sflstr.c \
          ../../../../../src/ta_common/imatix/sfl/sfltok.c \
          ../../../../../src/ta_common/imatix/sfl/sfldir.c \
          ../../../../../src/ta_common/imatix/sfl/sflfile.c \
          ../../../../../src/ta_common/trio/trionan.c \
          ../../../../../src/ta_common/ta_country_info.c \
          ../../../../../src/ta_common/ta_dict.c \
          ../../../../../src/ta_common/ta_global.c \
          ../../../../../src/ta_common/ta_list.c \
          ../../../../../src/ta_common/ta_memory.c \
          ../../../../../src/ta_common/ta_stream.c \
          ../../../../../src/ta_common/ta_string.c \
          ../../../../../src/ta_common/ta_system.c \
          ../../../../../src/ta_common/ta_timestamp.c \
          ../../../../../src/ta_common/ta_trace.c \
          ../../../../../src/ta_common/ta_retcode.c \
          ../../../../../src/ta_common/ta_version.c


# Compiler Options
INCLUDEPATH = ../../../../../src/ta_common \
              ../../../../../src/ta_common/bzip2 \
              ../../../../../src/ta_common/kazlib \
              ../../../../../src/ta_common/imatix/sfl \
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