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
LIBTARGET = ta_pm
cmd:TARGET = ta_pm_cmd
cmr:TARGET = ta_pm_cmr
cmp:TARGET = ta_pm_cmp
csd:TARGET = ta_pm_csd
csr:TARGET = ta_pm_csr
csp:TARGET = ta_pm_csp
cdr:TARGET = ta_pm_cdr
cdd:TARGET = ta_pm_cdd

# Output info
DESTDIR     = ../../../../../lib

# Files to process
SOURCES	= ../../../../../src/ta_pm/ta_pm.c \
          ../../../../../src/ta_pm/ta_pmvalue.c \
          ../../../../../src/ta_pm/ta_pmarray.c \
          ../../../../../src/ta_pm/ta_pmreport.c \
          ../../../../../src/ta_pm/ta_pmstring.c \
          ../../../../../src/ta_pm/ta_datalog.c \
          ../../../../../src/ta_pm/ta_instrument.c

# Compiler Options
INCLUDEPATH = ../../../../../src/ta_common \
              ../../../../../src/ta_pm \
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