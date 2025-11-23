CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
TARGET    := roi_surveillance
OUT       := $(BIN_DIR)/$(TARGET)

DS_PATH     := /opt/nvidia/deepstream/deepstream
GST_CFLAGS  := $(shell pkg-config --cflags gstreamer-1.0 gstreamer-pbutils-1.0)
GST_LIBS    := $(shell pkg-config --libs   gstreamer-1.0 gstreamer-pbutils-1.0)

INC := \
  -I$(SRC_DIR) \
  -I$(SRC_DIR)/config \
  -I$(SRC_DIR)/pipeline \
  -I$(SRC_DIR)/report \
  -I$(SRC_DIR)/roi \
  -I$(DS_PATH)/sources/includes \
  $(GST_CFLAGS)

LIBS := \
  $(GST_LIBS) \
  -L$(DS_PATH)/lib \
  -lnvdsgst_meta -lnvds_meta -lnvdsgst_helper \
  -lnvbufsurface -lnvbufsurftransform \
  -Wl,-rpath,$(DS_PATH)/lib

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Generar lista de archivos .d basada en los objetos (no buscar en disco)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean distclean help clobber

all: $(OUT)
	@echo "✔ build: $(OUT)"

$(OUT): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LIBS)

# Compilación: crea el directorio padre del .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

# Incluir dependencias solo si existen archivos .d
-include $(wildcard $(BUILD_DIR)/*/*.d) $(wildcard $(BUILD_DIR)/*.d)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "✔ objetos y .d limpiados"

distclean: clean
	@rm -rf $(BIN_DIR)
	@echo "✔ binarios limpiados"

# Limpieza dura: borra también cualquier directorio .d mal creado
clobber: distclean
	@find $(BUILD_DIR) -type d -name '.d' -exec rm -rf {} + 2>/dev/null || true
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✔ clobber completo"

help:
	@echo "Targets disponibles:"
	@echo "  make          - Compila el proyecto"
	@echo "  make clean    - Elimina objetos y dependencias"
	@echo "  make distclean- Elimina objetos y binarios"
	@echo "  make clobber  - Limpieza completa (incluye directorios .d)"
	@echo ""
	@echo "Uso:"
	@echo "  $(OUT) vi-file input.mp4 vo-file output.mp4 --time 5"
