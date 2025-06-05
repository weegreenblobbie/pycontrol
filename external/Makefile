CACTUS_LIB := cactus-rt/build/release/libcactus_rt.a
CACTUS_REPO := cactus-rt/.git/config
CATCH_REPO := Catch2/.git/config
CATCH_LIB := Catch2/build/installed/include/catch2/catch_user_config.hpp

.PHONY: all
all: $(CACTUS_LIB) $(CATCH_LIB)

$(CACTUS_REPO):
	git clone https://github.com/cactusdynamics/cactus-rt.git
	cd cactus-rt; \
		git checkout 3997bd8bae85452322b7c7cbfe1bf6a88223ac4c

$(CACTUS_LIB): $(CACTUS_REPO)
	cd cactus-rt; \
		make release

# Catch2 release 3.8.2.
$(CATCH_REPO):
	git clone https://github.com/catchorg/Catch2.git
	cd Catch2; \
		git checkout 2b60af89e23d28eefc081bc930831ee9d45ea58b

$(CATCH_LIB): $(CATCH_REPO)
	cd Catch2; \
		mkdir -p build && \
		cd build && \
		cmake .. -DCMAKE_INSTALL_PREFIX=./installed -DCATCH_BUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON && \
		make install -j 4

clean:
	@echo cleaning

real-clean:
	rm -tfv cactus-rt

# :mode=Makefile: