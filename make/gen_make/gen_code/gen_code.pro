TEMPLATE = app
CONFIG   -= qt

# Force this application to be a console application
CONFIG   -= windows
CONFIG   += console

# Identify the temp dir
cmd:OBJECTS_DIR = ../../../../../temp/cmd/gen_code
cmr:OBJECTS_DIR = ../../../../../temp/cmr/gen_code
csd:OBJECTS_DIR = ../../../../../temp/csd/gen_code
csr:OBJECTS_DIR = ../../../../../temp/csr/gen_code

# Output info
TARGET      = gen_code
DESTDIR     = ../../../../../bin

# Files to process

SOURCES = ../../../../../src/tools/gen_code/gen_code.c \
          ../../../../../src/ta_abstract/ta_abstract.c \
          ../../../../../src/ta_abstract/ta_def_ui.c \
          ../../../../../src/ta_abstract/tables/table_a.c \
          ../../../../../src/ta_abstract/tables/table_b.c \
          ../../../../../src/ta_abstract/tables/table_c.c \
          ../../../../../src/ta_abstract/tables/table_d.c \
          ../../../../../src/ta_abstract/tables/table_e.c \
          ../../../../../src/ta_abstract/tables/table_f.c \
          ../../../../../src/ta_abstract/tables/table_g.c \
          ../../../../../src/ta_abstract/tables/table_h.c \
          ../../../../../src/ta_abstract/tables/table_i.c \
          ../../../../../src/ta_abstract/tables/table_j.c \
          ../../../../../src/ta_abstract/tables/table_k.c \
          ../../../../../src/ta_abstract/tables/table_l.c \
          ../../../../../src/ta_abstract/tables/table_m.c \
          ../../../../../src/ta_abstract/tables/table_n.c \
          ../../../../../src/ta_abstract/tables/table_o.c \
          ../../../../../src/ta_abstract/tables/table_p.c \
          ../../../../../src/ta_abstract/tables/table_q.c \
          ../../../../../src/ta_abstract/tables/table_r.c \
          ../../../../../src/ta_abstract/tables/table_s.c \
          ../../../../../src/ta_abstract/tables/table_t.c \
          ../../../../../src/ta_abstract/tables/table_u.c \
          ../../../../../src/ta_abstract/tables/table_v.c \
          ../../../../../src/ta_abstract/tables/table_w.c \
          ../../../../../src/ta_abstract/tables/table_x.c \
          ../../../../../src/ta_abstract/tables/table_y.c \
          ../../../../../src/ta_abstract/tables/table_z.c


# Additional libraries
win32:TA_COMMON_CMD = ta_common_cmd.lib
win32:TA_COMMON_CMR = ta_common_cmr.lib
win32:TA_COMMON_CSD = ta_common_csd.lib
win32:TA_COMMON_CSR = ta_common_csr.lib

unix:TA_COMMON_CMD  = libta_common_cmd.a
unix:TA_COMMON_CMR  = libta_common_cmr.a
unix:TA_COMMON_CSD  = libta_common_csd.a
unix:TA_COMMON_CSR  = libta_common_csr.a

cmd:LIBS += ../../../../../lib/$$TA_COMMON_CMD
cmr:LIBS += ../../../../../lib/$$TA_COMMON_CMR
csd:LIBS += ../../../../../lib/$$TA_COMMON_CSD
csr:LIBS += ../../../../../lib/$$TA_COMMON_CSR

win32:LIBS += wininet.lib
unix:LIBS += -lcurl -lssl -lcrypto -ldl

# Compiler Options
INCLUDEPATH = ../../../../../include \
              ../../../../../src/ta_common \
              ../../../../../src/ta_common/imatix/sfl \
              ../../../../../src/ta_common/kazlib \
              ../../../../../src/ta_abstract \
              ../../../../../src/ta_abstract/tables \
              ../../../../../src/ta_abstract/frames

DEFINES *= TA_GEN_CODE

# debug/release dependent options.
debug:DEFINES  *= TA_DEBUG
debug:DEFINES  *= _DEBUG
DEFINES        += TA_SINGLE_THREAD
thread:DEFINES -= TA_SINGLE_THREAD


# Platform dependent options.
win32:DEFINES              *= WIN32
win32-msvc:DEFINES         *= _MBCS _LIB

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
