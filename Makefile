# Makefile para Sistema de Vigilancia ROI
# Universidad de Costa Rica - IE0301

CC      := g++
CFLAGS  := -Wall -std=c++11 -MMD -MP     
LDFLAGS :=

# Rutas
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
TARGET    := roi_surveillance
OUT       := $(BIN_DIR)/$(TARGET)

#DeepStream / GStreamer 
DS_PATH := /opt/nvidia/deepstream/deepstream

GSTREAMER_INCLUDES := $(shell pkg-config --cflags gstreamer-1.0)
GSTREAMER_LIBS     := $(shell pkg-config --libs   gstreamer-1.0)

INCLUDES := \
  -I$(SRC_DIR) \
  -I$(SRC_DIR)/config \
  -I$(SRC_DIR)/pipeline \
  -I$(SRC_DIR)/report \
  -I$(SRC_DIR)/roi \
  -I$(DS_PATH)/sources/includes \
  $(GSTREAMER_INCLUDES)

LIBS := \
  $(GSTREAMER_LIBS) \
  -L$(DS_PATH)/lib \
  -lnvdsgst_meta \
  -lnvds_meta \
  -lnvdsgst_helper \
  -lnvbufsurface \
  -lnvbufsurftransform \
  -Wl,-rpath,$(DS_PATH)/lib

# Sources
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

# targets
.PHONY: all clean distclean help tree

all: $(OUT)
	@echo "Compilación completa: $(OUT)"

$(OUT): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)

# Regla patrón: compilar cada .cpp a build/%.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/%
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Crear directorios espejo en build/ para cada subcarpeta de src
# Ej: build/config, build/pipeline, etc.
$(BUILD_DIR)/%:
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

# Dependencias automáticas (.d)
-include $(DEPS)

# Utilidades
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Limpieza de objetos y dependencias"

distclean: clean
	@rm -rf $(BIN_DIR)
	@echo "Limpio)"

help:
	@echo "Uso:"
	@echo "  make            - Compila el proyecto (binario en $(OUT))"
	@echo "  make clean      - Borra objetos y .d"
	@echo "  make distclean  - Borra también binarios"
	@echo "  make tree       - Muestra fuentes detectados"
	@echo ""
	@echo "Ejecutar:"
	@echo "  ./$(OUT) vi-file input.mp4 vo-file output.mp4 --time 5"

tree:
	@echo "Fuentes:" && printf "  %s\n" $(SOURCES)

