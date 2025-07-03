include ../config.mk

CACTUS_LIB := lib/libcactus_rt.a
CACTUS_REPO := cactus-rt/.git/config

CATCH_REPO := Catch2/.git/config
CATCH_LIB := lib/libCatch2.so

LIBGPHOTO2_REPO := libgphoto2/.git/config
LIBGPHOTO2_LIB := lib/libgphoto2.so

GPHOTO2CPP_LIB := include/gphoto2cpp/gphoto2cpp.h

MESON_VENV := venv/bin/meson

CWD := $(CURDIR)

MESON := $(cwd)$(MESON_VENV)

INSTALL_DIRS := $(CWD)/bin $(CWD)/include $(CWD)/lib $(CWD)/share

all: $(INSTALL_DIRS) $(CACTUS_LIB) $(CATCH_LIB) $(LIBGPHOTO2_LIB) $(GPHOTO2CPP_LIB)

$(INSTALL_DIRS):
	@echo "$(BUILD_COLOR)Making$(RESET) $@"
	$(SILENT)mkdir -p $@

# Cactus RT.
$(CACTUS_REPO):
	@echo "$(CLONE_COLOR)Cloning$(RESET) $@"
	$(SILENT)git clone https://github.com/cactusdynamics/cactus-rt.git
	$(SILENT)cd cactus-rt && \
		git checkout 3997bd8bae85452322b7c7cbfe1bf6a88223ac4c

$(CACTUS_LIB): $(CACTUS_REPO)
	@echo "$(CONFIGURE_COLOR)Configuring$(RESET) $@"
	$(SILENT)mkdir -p cactus_build && \
    cmake -S cactus-rt -B cactus_build \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DENABLE_CLANG_TIDY=OFF \
        -DENABLE_EXAMPLES=OFF \
        -DBUILD_DOCS=OFF \
        -DBUILD_TESTING=OFF \
        -DENABLE_TRACING=ON \
        -DCMAKE_INSTALL_PREFIX="$(CWD)" $(DEVNULL) && \
    echo "$(BUILD_COLOR)Building$(RESET) $@" && \
    cmake --build cactus_build -j $$(nproc) $(DEVNULL) && \
    echo "$(INSTALL_COLOR)Installing$(RESET) $@" && \
    cp -r cactus-rt/include/cactus_rt $(CWD)/include && \
    cp cactus_build/libcactus_rt.a $(CWD)/lib && \
    cp cactus_build/protos/libcactus_tracing_embedded_perfetto_protos.a $(CWD)/lib && \
    cp cactus_build/_deps/quill-build/quill/libquill.* $(CWD)/lib && \
    cp -r cactus_build/_deps/quill-src/quill/include/quill $(CWD)/include && \
    touch $@

# Catch2 release 3.8.2.
$(CATCH_REPO):
	@echo "$(CLONE_COLOR)Cloning$(RESET) $@"
	$(SILENT)git clone https://github.com/catchorg/Catch2.git
	$(SILENT)cd Catch2 && \
		git checkout 2b60af89e23d28eefc081bc930831ee9d45ea58b

$(CATCH_LIB): $(CATCH_REPO)
	@echo "$(CONFIGURE_COLOR)Configuring$(RESET) $@"
	$(SILENT)mkdir -p catch2_build && \
	cmake -S Catch2 -B catch2_build \
        -DCMAKE_INSTALL_PREFIX="$(CWD)" \
        -DCATCH_BUILD_TESTING=OFF \
        -DBUILD_SHARED_LIBS=ON $(DEVNULL) && \
	echo "$(BUILD_COLOR)Building$(RESET) $@" && \
	cmake --build catch2_build -j $$(nproc) $(DEVNULL) && \
	echo "$(INSTALL_COLOR)Installing$(RESET) $@" && \
	cmake --install catch2_build $(DEVNULL) && \
	touch $@

# libgphoto2.
$(LIBGPHOTO2_REPO):
	@echo "$(CLONE_COLOR)Cloning$(RESET) $@"
	$(SILENT)git clone https://github.com/gphoto/libgphoto2.git && \
	cd libgphoto2 && \
	   git checkout 9df7485aa54f2c3233e52e2a3efb1daf5cf6f967

$(MESON_VENV):
	@echo "$(INSTALL_COLOR)Installing$(RESET) $@"
	python3 -m venv venv && \
	. venv/bin/activate && \
	python3 -m pip install "meson>=1.4.0"

$(LIBGPHOTO2_LIB): $(LIBGPHOTO2_REPO) $(MESON_VENV)
	@echo "$(CONFIGURE_COLOR)Configuring$(RESET) $@"
	$(SILENT). venv/bin/activate && \
	cd libgphoto2 && \
	mkdir -p builddir && \
	meson setup builddir $(DEVNULL) && \
	cd builddir && \
	echo "$(BUILD_COLOR)Building$(RESET) $@" && \
	meson configure --prefix=$(CWD) --libdir=$(CWD)/lib --bindir=$(CWD)/bin && \
	meson compile $(DEVNULL) && \
	echo "$(INSTALL_COLOR)Installing$(RESET) $@" && \
	meson install  $(DEVNULL)

# gphoto2cpp - C++ wrapper.
$(GPHOTO2CPP_LIB): gphoto2cpp/include/gphoto2cpp/gphoto2cpp.h
	@echo "$(INSTALL_COLOR)Installing$(RESET) $@"
	$(SILENT)cp -r gphoto2cpp/include/gphoto2cpp $(CWD)/include


.PHONY: clean real-clen
clean:
	@echo "$(CLEAN_COLOR)Cleaning$(RESET) $(shell pwd)"
	$(SILENT)rm -rf $(INSTALL_DIRS) cactus_build catch2_build libgphoto2/builddir

real-clean:
	@echo "$(CLEAN_COLOR)Real Cleaning$(RESET) $(shell pwd)"
	$(SILENT)rm -tfv $(CACTUS_REPO) $(CACTUS_REPO) $(LIBGPHOTO2_REPO)

# :mode=Makefile: