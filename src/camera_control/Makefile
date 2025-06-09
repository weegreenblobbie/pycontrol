EXTERNAL_DIR = ../../external

GPHOTO2_INSTALL_DIR := $(EXTERNAL_DIR)/gphoto2cpp
CACTUS_INSTALL_DIR := $(EXTERNAL_DIR)/cactus-rt
CATCH2_INSTALL_DIR := $(EXTERNAL_DIR)/Catch2/build/installed


CC = g++
CPPINCLUDES = -I.. \
              -I$(GPHOTO2_INSTALL_DIR)/include \
              -I$(CACTUS_INSTALL_DIR)/include \
              -I$(CACTUS_INSTALL_DIR)/build/release/_deps/quill-src/quill/include \
              -I$(CATCH2_INSTALL_DIR)/include

CXXFLAGS = -std=c++20 -Wall -O2 -g $(CPPINCLUDES)

LINKFLAGS = \
    -L../common \
    -L$(CACTUS_INSTALL_DIR)/build/release \
	-L$(CACTUS_INSTALL_DIR)/build/release/_deps/quill-build/quill \
	-L$(CACTUS_INSTALL_DIR)/build/release/protos \
	-L$(CATCH2_INSTALL_DIR)/lib \
	-Wl,-rpath,$(abspath $(CATCH2_INSTALL_DIR)/lib)

LIBS = \
    -lcommon \
    -lgphoto2 \
    -lgphoto2_port \
    -lcactus_rt \
    -lquill \
    -lcactus_tracing_embedded_perfetto_protos \
	-lprotobuf \
	-lpthread \
	-lCatch2 \
	-lCatch2Main

HEADERS = $(wildcard *.h)
ALL_SOURCE = $(wildcard *.cc)
ALL_OBJECTS = $(wildcard *.o)

CAMERA_CONTROL_BIN := camera_control_bin
UNIT_TEST_BIN := unit_tests_bin

ALL_BIN := $(CAMERA_CONTROL_BIN) $(UNIT_TEST_BIN)

all: $(ALL_BIN)

CAMERA_CONTROL_BIN_SRC := $(filter-out %_uto.cc, $(ALL_SOURCE))
CAMERA_CONTROL_BIN_OBJS := $(CAMERA_CONTROL_BIN_SRC:.cc=.o)

UNIT_TEST_BIN_SRC := $(wildcard *_uto.cc)
UNIT_TEST_BIN_OBJS := $(UNIT_TEST_BIN_SRC:.cc=.o)

%.o: %.cc $(HEADERS)
	$(CC) $(CXXFLAGS) -c $< -o $@

$(CAMERA_CONTROL_BIN): $(CAMERA_CONTROL_BIN_OBJS) ../common/libcommon.a
	g++ -o $(CAMERA_CONTROL_BIN) $(CAMERA_CONTROL_BIN_OBJS) $(LINKFLAGS) $(LIBS)

$(UNIT_TEST_BIN): $(ALL_SOURCE) $(UNIT_TEST_BIN_OBJS) CameraSequenceFileReader.o ../common/libcommon.a
	g++ -o $(UNIT_TEST_BIN) $(UNIT_TEST_BIN_OBJS) CameraSequenceFileReader.o $(LINKFLAGS) $(LIBS)

test: $(UNIT_TEST_BIN)
	./$(UNIT_TEST_BIN)

clean:
	rm -f $(ALL_BIN) $(ALL_OBJECTS)

dump:
	@echo $(CAMERA_CONTROL_BIN)
	@echo $(CAMERA_CONTROL_BIN_SRC)
	@echo $(CAMERA_CONTROL_BIN_OBJS)
	@echo $(UNIT_TEST_BIN)
	@echo $(UNIT_TEST_BIN_SRC)
	@echo $(UNIT_TEST_BIN_OBJS)

# :mode=Makefile:
