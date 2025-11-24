#!/bin/bash

# Stream UDP por red a otro dispositivo
# Universidad de Costa Rica - IE0301

VIDEO="videosPrueba/videoCarros.mp4"
PORT=5000

echo "=============================================="
echo "  UDP Streaming por Red"
echo "=============================================="
echo ""

# Detectar IP local
echo "Detectando configuracion de red..."
echo ""
echo "Interfaces de red disponibles:"
ip -4 addr show | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | nl
echo ""

# Pedir IP del servidor (Jetson)
read -p "IP del servidor (Jetson Nano) [ENTER para auto-detectar]: " SERVER_IP
if [ -z "$SERVER_IP" ]; then
    SERVER_IP=$(hostname -I | awk '{print $1}')
    echo "IP del servidor detectada: $SERVER_IP"
fi

# Pedir IP del cliente
read -p "IP del cliente (dispositivo receptor): " CLIENT_IP
if [ -z "$CLIENT_IP" ]; then
    echo "ERROR: Debes especificar la IP del cliente"
    exit 1
fi

echo ""
echo "=============================================="
echo "CONFIGURACION:"
echo "=============================================="
echo "Servidor (Jetson): $SERVER_IP"
echo "Cliente:           $CLIENT_IP"
echo "Puerto:            $PORT"
echo "Video:             $VIDEO"
echo ""

# Crear archivo SDP para el cliente
SDP_FILE="stream_network.sdp"
cat > "$SDP_FILE" << EOF
v=0
o=- 0 0 IN IP4 $SERVER_IP
s=ROI Surveillance Network Stream
c=IN IP4 $CLIENT_IP
t=0 0
m=video $PORT RTP/AVP 96
a=rtpmap:96 H264/90000
EOF

echo "[OK] Archivo SDP creado: $SDP_FILE"
echo ""

# Verificar firewall
echo "Verificando firewall..."
if command -v ufw &> /dev/null; then
    UFW_STATUS=$(sudo ufw status | grep -i "Status: active")
    if [ ! -z "$UFW_STATUS" ]; then
        echo "WARNING: UFW firewall esta activo"
        echo "Abriendo puerto $PORT..."
        sudo ufw allow $PORT/udp
    fi
fi

echo ""
echo "=============================================="
echo "INSTRUCCIONES PARA EL CLIENTE:"
echo "=============================================="
echo ""
echo "1. Copia el archivo '$SDP_FILE' al cliente"
echo ""
echo "2. En el cliente ($CLIENT_IP), ejecuta:"
echo ""
echo "   # Opcion A: VLC"
echo "   vlc stream_network.sdp"
echo ""
echo "   # Opcion B: VLC directo (sin SDP)"
echo "   vlc rtp://@:$PORT"
echo ""
echo "   # Opcion C: GStreamer"
echo "   gst-launch-1.0 udpsrc port=$PORT ! \\"
echo "     application/x-rtp,encoding-name=H264,payload=96 ! \\"
echo "     rtph264depay ! h264parse ! avdec_h264 ! \\"
echo "     videoconvert ! autovideosink"
echo ""
echo "   # Opcion D: FFplay"
echo "   ffplay -fflags nobuffer -i rtp://$CLIENT_IP:$PORT"
echo ""
echo "=============================================="
echo ""
read -p "Presiona ENTER cuando el cliente este listo para recibir..."

echo ""
echo "Iniciando streaming a $CLIENT_IP:$PORT..."
echo ""

# Ejecutar el servidor
./bin/roi_surveillance \
    vi-file "$VIDEO" \
    --mode udp \
    --udp-host "$CLIENT_IP" \
    --udp-port "$PORT" \
    --time 5

echo ""
echo "=============================================="
echo "Stream finalizado"
echo ""
echo "Archivo SDP para el cliente: $SDP_FILE"
echo "Copia este archivo al cliente si es necesario"
echo "=============================================="
