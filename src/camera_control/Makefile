PROG := camera_control_bin

EXTERNAL_DIR = ../../external

CC = g++
CPPINCLUDES = -I.. \
              -I$(EXTERNAL_DIR)/gphoto2cpp/include \
              -I$(EXTERNAL_DIR)/cactus-rt/include \
              -I$(EXTERNAL_DIR)/cactus-rt/build/release/_deps/quill-src/quill/include

CXXFLAGS = -std=c++2a -Wall -O2 -g $(CPPINCLUDES)

LINKFLAGS = -L../common -lcommon -lgphoto2 -lgphoto2_port \
    -L$(EXTERNAL_DIR)/cactus-rt/build/release -lcactus_rt \
	-L$(EXTERNAL_DIR)/cactus-rt/build/release/_deps/quill-build/quill -lquill \
	-L$(EXTERNAL_DIR)/cactus-rt/build/release/protos -lcactus_tracing_embedded_perfetto_protos \
	-lprotobuf \
	-lpthread


SOURCES = $(wildcard *.cc)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cc=.o)

all: $(PROG)

%.o: %.cc $(HEADERS)
	$(CC) $(CXXFLAGS) -c $< -o $@

$(PROG): $(OBJECTS) ../common/libcommon.a
	g++ -o $(PROG) $(OBJECTS) $(LINKFLAGS)

clean:
	rm -f $(PROG) $(OBJECTS)

dump:
	@echo $(PROG)
	@echo $(SOURCES)
	@echo $(OBJECTS)

# :mode=Makefile:
