LIB := libcommon.a

AR = ar
CC = g++
CXXFLAGS = -Wall -O2 -g -I..

SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:.cc=.o)

all: $(LIB)

$(OBJECTS): $(SOURCES)
	$(CC) $(CXXFLAGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $(LIB) $(OBJECTS)

clean:
	rm -f $(LIB) $(OBJECTS)

# :mode=Makefile:
