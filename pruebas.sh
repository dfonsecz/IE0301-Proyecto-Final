#!/usr/bin/env bash
################################################################################
# run_tests.sh - Pruebas para Sistema de Vigilancia ROI
# UCR - IE0301
################################################################################
set -euo pipefail

# --- Colores (solo para mensajes) ---
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# --- Defaults ---
EXEC_DEFAULT="bin/roi_surveillance"   # Binario generado por el Makefile propuesto
INPUT_DEFAULT="/opt/nvidia/deepstream/deepstream/samples/streams/sample_1080p_h264.mp4"
OUT_DEFAULT="test_results"

EXEC="$EXEC_DEFAULT"
INPUT_VIDEO="$INPUT_DEFAULT"
RESULTS_DIR="$OUT_DEFAULT"

usage() {
  cat <<EOF
Uso: $(basename "$0") [-e EXECUTABLE] [-i INPUT_VIDEO] [-o OUTPUT_DIR]
  -e  Ruta al ejecutable (default: $EXEC_DEFAULT)
  -i  Ruta al video de entrada (default: DeepStream sample)
  -o  Directorio de salida (default: $OUT_DEFAULT)
  -h  Ayuda
Ejemplo:
  $(basename "$0") -e bin/roi_surveillance -i /ruta/video.mp4 -o results
EOF
}

while getopts ":e:i:o:h" opt; do
  case "$opt" in
    e) EXEC="$OPTARG" ;;
    i) INPUT_VIDEO="$OPTARG" ;;
    o) RESULTS_DIR="$OPTARG" ;;
    h) usage; exit 0 ;;
    \?) echo -e "${RED}Opción inválida: -$OPTARG${NC}"; usage; exit 2 ;;
  esac
done

echo "=== Sistema de Pruebas - Vigilancia ROI ==="
echo ""

# --- Verificaciones previas (por qué: fallar temprano con mensajes claros) ---
if [[ ! -f "$EXEC" ]]; then
  echo -e "${RED}Error: No se encuentra el ejecutable '$EXEC'${NC}"
  echo "Ejecute 'make' para compilar o use -e para indicar la ruta."
  exit 1
fi

mkdir -p "$RESULTS_DIR"

if [[ ! -f "$INPUT_VIDEO" ]]; then
  echo -e "${YELLOW}Advertencia: Video predeterminado no encontrado:${NC} $INPUT_VIDEO"
  read -rp "Ruta del video de entrada: " INPUT_VIDEO
  if [[ -z "${INPUT_VIDEO}" || ! -f "$INPUT_VIDEO" ]]; then
    echo -e "${RED}Error: archivo de video inválido${NC}"; exit 1
  fi
fi
echo -e "${GREEN}Video de entrada:${NC} $INPUT_VIDEO"
echo -e "${GREEN}Ejecutable:${NC} $EXEC"
echo -e "${GREEN}Resultados en:${NC} $RESULTS_DIR"
echo ""

# --- Helper para ejecutar escenarios ---
run_scenario() {
  local name="$1" left="$2" top="$3" width="$4" height="$5" time_secs="$6" mode="$7" extra_args="${8:-}"
  local base="scenario_${name}"
  local report="$RESULTS_DIR/report_${base}.txt"
  local output="$RESULTS_DIR/output_${base}.mp4"

  echo -e "${YELLOW}=== ESCENARIO ${name} ===${NC}"
  echo "ROI: left=$left top=$top width=$width height=$height  |  time=$time_secs  |  mode=$mode"

  if [[ "$mode" == "video" ]]; then
    "$EXEC" vi-file "$INPUT_VIDEO" \
      --left "$left" --top "$top" --width "$width" --height "$height" \
      --time "$time_secs" \
      --file-name "$report" \
      vo-file "$output" \
      --mode video $extra_args
    echo -e "${GREEN}OK:${NC} $output"
  else
    "$EXEC" vi-file "$INPUT_VIDEO" \
      --left "$left" --top "$top" --width "$width" --height "$height" \
      --time "$time_secs" \
      --file-name "$report" \
      --mode udp $extra_args
    echo -e "${GREEN}OK:${NC} UDP streaming finalizado"
  fi

  echo "Reporte: $report"
  echo ""
}

################################################################################
# ESCENARIOS
################################################################################

# 1) Objeto NO supera tiempo máximo
run_scenario "1_no_supera_tiempo" 0.35 0.35 0.30 0.30 10 "video"

# 2) Objeto SÍ supera tiempo máximo
run_scenario "2_supera_tiempo"     0.35 0.35 0.30 0.30  2 "video"

# 3) ROI grande para múltiples objetos
run_scenario "3_multiples_obj"     0.15 0.15 0.70 0.70  3 "video"

# 4) Streaming UDP
echo -e "${YELLOW}=== Nota UDP ===${NC}"
echo "Para visualizar, abre otra terminal y ejecuta:"
echo "  gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink"
read -rp "Presiona Enter para iniciar el escenario UDP (Ctrl+C para detener el viewer cuando corresponda)..."
run_scenario "4_udp"               0.40 0.40 0.20 0.20  5 "udp" "--udp-host 127.0.0.1 --udp-port 5000"

################################################################################
# RESUMEN
################################################################################
echo -e "${GREEN}=== RESUMEN DE PRUEBAS ===${NC}"
echo "Directorio: $RESULTS_DIR"
echo ""
echo "Videos generados:"
ls -lh "$RESULTS_DIR"/*.mp4 2>/dev/null || echo "  (ninguno)"
echo ""
echo "Reportes generados:"
ls -lh "$RESULTS_DIR"/*.txt 2>/dev/null || echo "  (ninguno)"
echo ""

# Mostrar contenido de reportes
shopt -s nullglob
for report in "$RESULTS_DIR"/report_scenario_*.txt; do
  echo -e "${YELLOW}--- $(basename "$report") ---${NC}"
  cat "$report" || true
  echo ""
done
shopt -u nullglob

echo -e "${GREEN}¡Todas las pruebas completadas!${NC}"
echo ""
echo "Para reproducir rápidamente:"
echo "  gst-play-1.0 $RESULTS_DIR/output_scenario_1_no_supera_tiempo.mp4"

