TEMPLATE  = app
CONFIG   -= qt


# Force this application to be a console application
CONFIG   -= windows
CONFIG   += console

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd/ta_yahoo
cmr:OBJECTS_DIR = ../../../../../temp/cmr/ta_yahoo
csd:OBJECTS_DIR = ../../../../../temp/csd/ta_yahoo
csr:OBJECTS_DIR = ../../../../../temp/csr/ta_yahoo
cdr:OBJECTS_DIR = ../../../../../temp/cdr/ta_yahoo
cdd:OBJECTS_DIR = ../../../../../temp/cdd/ta_yahoo

# Output info
TARGET      = ta_yahoo
DESTDIR     = ../../../../../bin

# Files to process
SOURCES	= ../../../../../src/demo/ta_yahoo/ta_yahoo.c 

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
              ../../../../../include \
              ../../../../../src/ta_common/imatix/sfl

# debug/release dependent options.
debug:DEFINES   *= TA_DEBUG
debug:DEFINES   *= _DEBUG
DEFINES        += TA_SINGLE_THREAD
thread:DEFINES -= TA_SINGLE_THREAD


# Platform dependent options.
win32:DEFINES         *= WIN32
win32-msvc:DEFINES    *= _MBCS _LIB
cygwin-g++:LIBS       -= -ldl
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
