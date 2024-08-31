PROG := control_bin

all: $(PROG)

$(PROG).o: $(PROG).cc
	g++ --std=c++17 -ggdb3 -O2 -c $(PROG).cc -o $(PROG).o \
	-I../../external/cactus-rt/include \
	-I../../external/cactus-rt/build/release/_deps/quill-src/quill/include

$(PROG): $(PROG).o
	g++ --std=c++17 -o $(PROG) $(PROG).o \
		-L../../external/cactus-rt/build/release -lcactus_rt \
		-L../../external/cactus-rt/build/release/_deps/quill-build/quill -lquill \
		-L../../external/cactus-rt/build/release/protos -lcactus_tracing_embedded_perfetto_protos \
		-lprotobuf \
		-lpthread

clean:
	rm -f $(PROG) $(PROG).o

# :mode=makefile: