LIB := cactus-rt/build/release/libcactus_rt.a
REPO := cactus-rt/.git/config

.PHONY: all
all: $(LIB)

$(REPO):
	git clone https://github.com/cactusdynamics/cactus-rt.git
	cd cactus-rt; \
		git checkout 3997bd8bae85452322b7c7cbfe1bf6a88223ac4c

$(LIB): cactus-rt/.git/config
	cd cactus-rt; \
		make release

clean:
	@echo cleaning

real-clean:
	rm -tfv cactus-rt

# :mode=Makefile: