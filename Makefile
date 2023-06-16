CMAKE   := cmake
XXD     := xxd
CFLAGS  := -O3 -Wall -ffunction-sections -fdata-sections
LDFLAGS := -Wl,--as-needed -Wl,--gc-sections -s

BIN = launcher
FLTK_PREFIX = $(CURDIR)/build/usr
FLTK_CONFIG = $(FLTK_PREFIX)/bin/fltk-config

VERBOSE ?= $(V)

CMAKE_OPTIONS := -DCMAKE_BUILD_TYPE=Release
CMAKE_OPTIONS += -DCMAKE_CXX_FLAGS="$(CFLAGS)"
CMAKE_OPTIONS += -DCMAKE_C_FLAGS="$(CFLAGS)"
CMAKE_OPTIONS += -DCMAKE_INSTALL_PREFIX=$(FLTK_PREFIX)

# turn off to make the build faster
CMAKE_OPTIONS += -DFLTK_BUILD_EXAMPLES=OFF
CMAKE_OPTIONS += -DFLTK_BUILD_TEST=OFF
CMAKE_OPTIONS += -DFLTK_BUILD_FLUID=OFF
CMAKE_OPTIONS += -DFLTK_BUILD_FLTK_OPTIONS=OFF

# use xft or else the fonts will look ugly
CMAKE_OPTIONS += -DOPTION_USE_XFT=ON

# use the system versions of these libraries
CMAKE_OPTIONS += -DOPTION_USE_SYSTEM_LIBPNG=ON
CMAKE_OPTIONS += -DOPTION_USE_SYSTEM_ZLIB=ON

# features we don't need
CMAKE_OPTIONS += -DOPTION_PRINT_SUPPORT=OFF
CMAKE_OPTIONS += -DOPTION_USE_GL=OFF
CMAKE_OPTIONS += -DOPTION_USE_KDIALOG=OFF
CMAKE_OPTIONS += -DOPTION_USE_SVG=OFF



all: $(BIN)

clean:
	-rm -f res.h $(BIN)

distclean: clean
	-rm -rf build

maintainer-clean: distclean
	-rm -rf fltk

$(BIN): $(FLTK_CONFIG) launcher.cpp launcher.hpp res.h
	$(FLTK_CONFIG) --use-images --compile launcher.cpp $(LDFLAGS)

res.h: input-gaming.png
	$(XXD) -i $< | sed -e 's|unsigned|const unsigned|g' > $@

$(FLTK_CONFIG):
	mkdir -p build && cd build && \
  $(CMAKE) ../fltk $(CMAKE_OPTIONS) && \
  $(MAKE) VERBOSE=$(VERBOSE) && \
  $(MAKE) install

