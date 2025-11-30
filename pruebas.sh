#!/bin/bash

# Script simple de pruebas de ROI
# Universidad de Costa Rica - IE0301

VIDEO_INPUT="videosPrueba/videoCarros.mp4"
OUTPUT_DIR="resultados_pruebas"
REPORTS_DIR="reportes_pruebas"

echo "=============================================="
echo "  Pruebas de ROI - 3 Posiciones"
echo "=============================================="
echo ""

# Verificar ejecutable
if [ ! -f "./bin/roi_surveillance" ]; then
    echo "ERROR: Ejecutable no encontrado. Ejecuta 'make' primero"
    exit 1
fi

# Verificar video
if [ ! -f "$VIDEO_INPUT" ]; then
    echo "ERROR: Video no encontrado: $VIDEO_INPUT"
    exit 1
fi

# Crear directorios
mkdir -p "$OUTPUT_DIR" "$REPORTS_DIR"

echo "Video: $VIDEO_INPUT"
echo ""
echo "Las pruebas procesaran el video con el ROI en 3 posiciones:"
echo "  1. Centro (default)"
echo "  2. Esquina superior izquierda"
echo "  3. Esquina inferior derecha"
echo ""
read -p "Presiona ENTER para iniciar..."

# Prueba 1: Centro
echo ""
echo "=============================================="
echo "[1/3] ROI Centrado"
echo "=============================================="
./bin/roi_surveillance \
    vi-file "$VIDEO_INPUT" \
    vo-file "$OUTPUT_DIR/roi_centro.mp4" \
    --file-name "$REPORTS_DIR/roi_centro.txt" \
    --width 0.4 --height 0.4 --center \
    --time 5

if [ $? -eq 0 ]; then
    echo "[OK] Video generado: $OUTPUT_DIR/roi_centro.mp4"
else
    echo "[ERROR] Fallo en procesamiento"
fi

# Prueba 2: Esquina superior izquierda
echo ""
echo "=============================================="
echo "[2/3] ROI Esquina Superior Izquierda"
echo "=============================================="
./bin/roi_surveillance \
    vi-file "$VIDEO_INPUT" \
    vo-file "$OUTPUT_DIR/roi_superior_izq.mp4" \
    --file-name "$REPORTS_DIR/roi_superior_izq.txt" \
    --width 0.35 --height 0.35 \
    --left 0.1 --top 0.1 \
    --time 5

if [ $? -eq 0 ]; then
    echo "[OK] Video generado: $OUTPUT_DIR/roi_superior_izq.mp4"
else
    echo "[ERROR] Fallo en procesamiento"
fi

# Prueba 3: Esquina inferior derecha
echo ""
echo "=============================================="
echo "[3/3] ROI Esquina Inferior Derecha"
echo "=============================================="
./bin/roi_surveillance \
    vi-file "$VIDEO_INPUT" \
    vo-file "$OUTPUT_DIR/roi_inferior_der.mp4" \
    --file-name "$REPORTS_DIR/roi_inferior_der.txt" \
    --width 0.35 --height 0.35 \
    --left 0.55 --top 0.55 \
    --time 5

if [ $? -eq 0 ]; then
    echo "[OK] Video generado: $OUTPUT_DIR/roi_inferior_der.mp4"
else
    echo "[ERROR] Fallo en procesamiento"
fi

# Resumen
echo ""
echo "=============================================="
echo "Pruebas completadas!"
echo "=============================================="
echo ""
echo "Videos generados:"
ls -lh "$OUTPUT_DIR"/*.mp4 2>/dev/null | awk '{print "  "$9" ("$5")"}'
echo ""
echo "Reportes generados:"
ls -lh "$REPORTS_DIR"/*.txt 2>/dev/null | awk '{print "  "$9" ("$5")"}'
echo ""
echo "=============================================="
