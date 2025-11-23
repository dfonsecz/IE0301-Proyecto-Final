#!/bin/bash

# Script de configuracion para monitoreo de recursos
# Universidad de Costa Rica - IE0301

echo "Configurando herramientas de monitoreo..."
echo ""

# Verificar si estamos en Jetson
if [ -f /etc/nv_tegra_release ]; then
    echo "[OK] Jetson Nano detectado"
else
    echo "WARNING: No se detecto Jetson Nano, algunas funciones pueden no estar disponibles"
fi

# Verificar tegrastats
echo ""
echo "Verificando tegrastats..."
if command -v tegrastats &> /dev/null; then
    echo "[OK] tegrastats encontrado"
    tegrastats --help 2>&1 | head -n 5
else
    echo "ERROR: tegrastats no encontrado"
    echo "   Deberia estar en /usr/bin/tegrastats en Jetson Nano"
fi

# Instalar matplotlib
echo ""
echo "Instalando matplotlib para Python..."
if python3 -c "import matplotlib" 2>/dev/null; then
    echo "[OK] matplotlib ya esta instalado"
else
    echo "Instalando matplotlib..."
    sudo apt-get update
    sudo apt-get install -y python3-matplotlib python3-numpy
    
    if [ $? -eq 0 ]; then
        echo "[OK] matplotlib instalado correctamente"
    else
        echo "ERROR: Error instalando matplotlib"
        echo "   Intenta manualmente: sudo apt-get install python3-matplotlib"
    fi
fi

# Crear directorios necesarios
echo ""
echo "Creando directorios..."
mkdir -p videosPrueba resultados reportes stats plots
echo "[OK] Directorios creados"

# Dar permisos a los scripts
echo ""
echo "Configurando permisos..."
chmod +x quick_test.sh 2>/dev/null
chmod +x test_videos.sh 2>/dev/null
chmod +x test_videos_advanced.sh 2>/dev/null
chmod +x plot_stats.py 2>/dev/null
echo "[OK] Permisos configurados"

# Verificar compilacion
echo ""
echo "Verificando compilacion..."
if [ -f bin/roi_surveillance ]; then
    echo "[OK] Ejecutable encontrado: bin/roi_surveillance"
else
    echo "WARNING: Ejecutable no encontrado"
    echo "   Ejecuta 'make' para compilar el proyecto"
fi

echo ""
echo "=============================================="
echo "Configuracion completa!"
echo "=============================================="
echo ""
