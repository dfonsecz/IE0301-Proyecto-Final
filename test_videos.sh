#!/bin/bash

# Script rápido para procesar videos con monitoreo de recursos
# Uso: ./quick_test.sh

INPUT="videosPrueba"
OUTPUT="resultados"
REPORTS="reportes"
STATS_DIR="stats"

mkdir -p "$OUTPUT" "$REPORTS" "$STATS_DIR"

echo "Procesando videos en $INPUT..."
echo "Monitoreando recursos con tegrastats..."
echo ""

# Verificar que tegrastats existe
if ! command -v tegrastats &> /dev/null; then
    echo "WARNING: tegrastats no encontrado, continuando sin monitoreo"
    MONITOR=false
else
    MONITOR=true
fi

count=1
for video in "$INPUT"/*.mp4; do
    [ -f "$video" ] || continue
    
    name=$(basename "$video" .mp4)
    stats_file="$STATS_DIR/${name}_stats.log"
    
    echo "=============================================="
    echo "[$count] $name"
    echo "=============================================="
    
    # Iniciar tegrastats en background
    if [ "$MONITOR" = true ]; then
        echo "Monitoreando recursos -> $stats_file"
        tegrastats --interval 500 > "$stats_file" 2>&1 &
        TEGRASTATS_PID=$!
        sleep 1
    fi
    
    # Timestamp de inicio
    start_time=$(date +%s)
    
    # Ejecutar el procesamiento
    ./bin/roi_surveillance \
        vi-file "$video" \
        vo-file "$OUTPUT/${name}_output.mp4" \
        --file-name "$REPORTS/${name}_report.txt" \
        --time 3 \
        2>&1 | grep -E "(ROI|Frame|Detected|End of stream|Error)"
    
    exit_code=$?
    
    # Timestamp de fin
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    # Detener tegrastats
    if [ "$MONITOR" = true ] && [ -n "$TEGRASTATS_PID" ]; then
        kill $TEGRASTATS_PID 2>/dev/null
        wait $TEGRASTATS_PID 2>/dev/null
        
        # Agregar metadata al archivo de stats
        echo "# Video: $name" >> "$stats_file"
        echo "# Duracion: ${duration}s" >> "$stats_file"
        echo "# Timestamp: $(date)" >> "$stats_file"
    fi
    
    # Verificar resultado
    if [ -f "$OUTPUT/${name}_output.mp4" ] && [ $exit_code -eq 0 ]; then
        size=$(du -h "$OUTPUT/${name}_output.mp4" | cut -f1)
        echo "[OK] (${duration}s, $size)"
    else
        echo "[ERROR] (${duration}s)"
    fi
    echo ""
    
    ((count++))
done

echo "=============================================="
echo "Completado! Revisa:"
echo "   Videos:   $OUTPUT/"
echo "   Reportes: $REPORTS/"
if [ "$MONITOR" = true ]; then
    echo "   Stats:    $STATS_DIR/"
fi
echo "=============================================="
