LIB := libcommon.a

AR = ar
CC = g++
CXXFLAGS = -std=c++2a -Wall -O2 -g -I..

SOURCES = $(wildcard *.cc)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.cc=.o)

all: $(LIB)

%.o: %.cc $(HEADERS)
	$(CC) $(CXXFLAGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $(LIB) $(OBJECTS)

clean:
	rm -f $(LIB) $(OBJECTS)

# :mode=Makefile:
