PROG := camera_control_bin

CC = g++
CXXFLAGS = -std=c++2a -Wall -O2 -g -I.. -I../../external/gphoto2cpp/include
LINKFLAGS = -L../common -lcommon -lgphoto2 -lgphoto2_port

SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:.cc=.o)

all: $(PROG)

%.o: %.cc
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
