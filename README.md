# IE0301 Proyecto Final - Sistema de Vigilancia con ROI

## Descripción

Sistema de vigilancia inteligente basado en visión por computador e inteligencia artificial para la detección y seguimiento automático de vehículos en zonas específicas. El sistema utiliza NVIDIA DeepStream SDK para realizar inferencia en tiempo real sobre streams de video, identificando vehículos y monitoreando su permanencia dentro de una Región de Interés (ROI) configurable.

### Características principales

- Detección de vehículos en tiempo real usando modelos pre-entrenados (ResNet10)
- ROI configurable con posicionamiento y dimensiones ajustables
- Sistema de alertas basado en tiempo de permanencia en la ROI
- Seguimiento multi-objeto con NvDCF tracker
- Generación automática de reportes con estadísticas de detección
- Soporte para múltiples modos de salida: archivo de video, streaming UDP/RTP y display local
- Visualización con código de colores:
  - Verde: Vehículos fuera de la ROI
  - Naranja: Vehículos dentro de la ROI (tiempo < límite)
  - Rosa: Vehículos en alerta (tiempo > límite)
  - Efecto de parpadeo al activarse la alerta

## Índice

1. [Especificaciones de hardware](#especificaciones-de-hardware)
2. [Especificaciones de software](#especificaciones-de-software)
3. [Algoritmo y arquitectura](#algoritmo-y-arquitectura)
4. [Estructura del proyecto](#estructura-del-proyecto)
5. [Prerequisitos](#prerequisitos)
6. [Instalación](#instalación)
7. [Compilación](#compilación)
8. [Cómo utilizar](#cómo-utilizar)
9. [Scripts de prueba](#scripts-de-prueba)
10. [Monitoreo de recursos](#monitoreo-de-recursos)
11. [Licencia](#licencia)
12. [Contacto](#contacto)

## Especificaciones de hardware

- **SoC:** Tegra X1
- **SoM:** NVIDIA Jetson Nano
- **Carrier board:** NVIDIA Jetson Nano Developer Kit
- **Memoria:** 4GB LPDDR4
- **GPU:** 128-core Maxwell

## Especificaciones de software

- **BSP:** JetPack 4.6.4
- **Sistema operativo:** Ubuntu 18.04 LTS
- **Multimedia framework:** GStreamer 1.14.5
- **Video analytics / AI framework:** NVIDIA DeepStream SDK 6.0
- **Compilador:** g++ 7.5.0 con soporte C++17
- **Librerías adicionales:** gst-pbutils, CUDA 10.2

## Algoritmo y arquitectura

### Pipeline de procesamiento

El sistema utiliza un pipeline de DeepStream compuesto por los siguientes elementos:

1. **Decodificación de video:** nvv4l2decoder para aceleración por hardware
2. **Multiplexado de streams:** nvstreammux con resolución dinámica detectada
3. **Inferencia primaria:** nvinfer con modelo ResNet10 pre-entrenado
4. **Tracking multi-objeto:** nvtracker con algoritmo NvDCF
5. **Conversión de formato:** nvvideoconvert
6. **Overlay en pantalla:** nvdsosd para visualización de bounding boxes y ROI
7. **Codificación:** nvv4l2h264enc para salida de video

### Detección y clasificación

El modelo ResNet10 detecta las siguientes clases de objetos:
- **class_id 0:** Car (Automóvil)
- **class_id 1:** Person (Persona) - Solo detectada, no rastreada
- **class_id 2:** Bicycle (Bicicleta)
- **class_id 5:** Bus
- **class_id 7:** Truck (Camión)

Solo los vehículos (cars, bicycles, buses, trucks) son rastreados y pueden generar alertas.

### Lógica de seguimiento

El sistema implementa una máquina de estados para cada vehículo:

- **STATE_OUTSIDE:** Vehículo fuera de la ROI
- **STATE_INSIDE:** Vehículo dentro de la ROI, temporizador activo
- **STATE_ALERT:** Vehículo ha excedido el tiempo límite en la ROI

La verificación de posición se basa en el centro del bounding box del objeto.

## Estructura del proyecto

```
IE0301-Proyecto-Final/
├── src/
│   ├── main.cpp                    # Punto de entrada de la aplicación
│   ├── video_utils.h/cpp           # Detección automática de resolución
│   ├── config/
│   │   ├── app_config.hpp/cpp      # Parser de argumentos CLI
│   │   └── track_info.hpp/cpp      # Lógica de tracking y ROI
│   ├── pipeline/
│   │   └── pipeline.hpp/cpp        # Construcción del pipeline GStreamer
│   ├── roi/
│   │   └── render.h/cpp            # Renderizado del ROI y overlays
│   └── report/
│       └── report.hpp/cpp          # Generación de reportes
├── build/                          # Archivos objeto (generado)
├── bin/                            # Ejecutable (generado)
├── videosPrueba/                   # Videos de entrada para pruebas
├── resultados/                     # Videos procesados (generado)
├── reportes/                       # Reportes de detección (generado)
├── stats/                          # Logs de tegrastats (generado)
├── plots/                          # Gráficos de rendimiento (generado)
├── pruebas.sh                      # Script para probar configuraciones de ROI
├── test_videos.sh                  # Script para procesamiento batch
├── test_udp_network.sh             # Script para streaming UDP
├── plot_stats.py                   # Visualizador de estadísticas
├── Makefile                        # Sistema de compilación
└── README.md                       # Este archivo
```

## Prerequisitos

### Software requerido

- NVIDIA JetPack 4.6.4 o superior
- DeepStream SDK 6.0
- GStreamer 1.14.5+
- Python 3.6+ (para scripts de monitoreo)
- VLC o FFplay (opcional, para visualización de streams UDP)

### Dependencias de Python

```bash
sudo apt-get install python3-matplotlib python3-numpy
```

## Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/dfonsecz/IE0301-Proyecto-Final.git
cd IE0301-Proyecto-Final
```

### 2. Crear directorios necesarios

```bash
mkdir -p videosPrueba resultados reportes stats plots build bin
```

### 3. Verificar instalación de DeepStream

```bash
ls /opt/nvidia/deepstream/deepstream/
```

Si DeepStream no está instalado, siga la [guía oficial de NVIDIA](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Quickstart.html).

## Compilación

### Compilar el proyecto

```bash
make clean
make
```

El ejecutable se generará en `bin/roi_surveillance`.

### Opciones del Makefile

- `make` - Compila el proyecto
- `make clean` - Elimina archivos objeto
- `make distclean` - Elimina objetos y binarios
- `make help` - Muestra ayuda

## Cómo utilizar

### Sintaxis básica

```bash
./bin/roi_surveillance vi-file <input.mp4> [opciones]
```

### Opciones de línea de comandos

#### Configuración del ROI

- `--width <0-1>` - Ancho del ROI normalizado (default: 0.4)
- `--height <0-1>` - Alto del ROI normalizado (default: 0.4)
- `--left <0-1>` - Posición X del ROI normalizado (default: centrado)
- `--top <0-1>` - Posición Y del ROI normalizado (default: centrado)
- `--center` - Forzar centrado automático del ROI

#### Parámetros de detección

- `--time <segundos>` - Tiempo máximo en ROI antes de alerta (default: 5)

#### Modos de salida

- `--mode video` - Guardar a archivo de video (default)
- `--mode udp` - Streaming por UDP/RTP
- `vo-file <archivo>` - Archivo de salida (modo video)

#### Opciones de streaming UDP

- `--udp-host <IP>` - Dirección IP destino (default: 127.0.0.1)
- `--udp-port <puerto>` - Puerto UDP (default: 5000)

#### Otras opciones

- `--file-name <archivo>` - Nombre del archivo de reporte (default: report.txt)

### Ejemplos de uso

#### Procesamiento básico con salida a archivo

```bash
./bin/roi_surveillance vi-file videosPrueba/video.mp4 vo-file output.mp4
```

#### ROI personalizado con tiempo de 10 segundos

```bash
./bin/roi_surveillance vi-file input.mp4 vo-file output.mp4 \
  --width 0.5 --height 0.5 --center --time 10
```

#### Streaming UDP a otra computadora

```bash
./bin/roi_surveillance vi-file input.mp4 \
  --mode udp --udp-host 192.168.1.100 --udp-port 5000
```

#### ROI en posición específica

```bash
./bin/roi_surveillance vi-file input.mp4 vo-file output.mp4 \
  --left 0.2 --top 0.3 --width 0.6 --height 0.4 --time 8
```

## Scripts de prueba

El proyecto incluye varios scripts para facilitar las pruebas:

### pruebas.sh - Pruebas de configuración de ROI

Script interactivo para probar diferentes tamaños y posiciones del ROI.

```bash
./pruebas.sh
```

Permite probar:
- ROI pequeño, mediano y grande
- Diferentes posiciones (esquinas, centro)
- Diferentes tiempos de alerta

### test_videos.sh - Procesamiento batch

Procesa todos los videos en `videosPrueba/` de forma automática.

```bash
./test_videos.sh
```

Características:
- Procesa todos los archivos .mp4 en el directorio
- Genera videos de salida en `resultados/`
- Crea reportes individuales en `reportes/`
- Muestra estadísticas de éxito/fallo

### test_udp_network.sh - Streaming por red

Script para configurar y probar streaming UDP a otros dispositivos.

```bash
./test_udp_network.sh
```

Funcionalidad:
- Detecta automáticamente la IP del servidor
- Solicita la IP del cliente
- Genera archivo SDP para el cliente
- Configura firewall si es necesario
- Proporciona instrucciones para el cliente

#### Recepción del stream en el cliente

Opción 1 - VLC:
```bash
vlc rtp://@:5000
```

Opción 2 - GStreamer:
```bash
gst-launch-1.0 udpsrc port=5000 ! \
  application/x-rtp,encoding-name=H264,payload=96 ! \
  rtph264depay ! h264parse ! avdec_h264 ! \
  videoconvert ! autovideosink
```

Opción 3 - FFplay:
```bash
ffplay -fflags nobuffer -i rtp://@:5000
```

## Monitoreo de recursos

### Captura de estadísticas

El sistema puede monitorear el uso de recursos durante el procesamiento usando `tegrastats`:

```bash
# El script quick_test.sh captura automáticamente estadísticas
./test_videos.sh
```

Los logs se guardan en `stats/` con formato `{video}_stats.log`.

### Visualización de estadísticas

Genera gráficos de uso de CPU, GPU, RAM y temperatura:

```bash
# Procesar todos los logs
python3 plot_stats.py

# Procesar un archivo específico
python3 plot_stats.py stats/video_stats.log

# Solo generar gráfico combinado
python3 plot_stats.py --all
```

Los gráficos se generan en `plots/` en formato PNG (300 DPI).

### Métricas capturadas

- Uso de CPU por core y promedio
- Uso de GPU (frecuencia y porcentaje)
- Consumo de RAM (MB y porcentaje)
- Temperatura del CPU (°C)
- Consumo de energía (Watts)

## Formato del reporte

El sistema genera un reporte de texto con el siguiente formato:

```
ROI: left: 249 top: 139 width: 332 height: 185
Max time: 5s
Detected: 15 (3)
0:05 car time 8s alert
0:12 truck time 6s alert
0:25 car time 3s
```

Donde:
- Primera línea: Coordenadas del ROI en píxeles
- Segunda línea: Tiempo máximo configurado
- Tercera línea: Total detectado (alertas generadas)
- Líneas siguientes: Timestamp, clase de vehículo, tiempo en ROI, estado de alerta

## Solución de problemas

### Error: "Resource not found"

Verificar rutas de configuración de DeepStream:
```bash
ls /opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/
```

### Stream UDP no funciona

1. Verificar conectividad de red
2. Verificar firewall: `sudo ufw allow 5000/udp`
3. Iniciar cliente antes que el servidor
4. Verificar que el puerto no esté en uso

## Licencia

Este proyecto utiliza una licencia del MIT. Los detalles pueden encontrarse en [LICENSE](https://github.com/dfonsecz/IE0301-Proyecto-Final/blob/main/LICENSE).

## Contacto

- Byron Arguedas: byronarglop@gmail.com
- Daniela Fonseca: daniela.fonsecazumbado@gmail.com
- Mariana Solís: solisfanny2021@gmail.com

## Referencias

- [NVIDIA DeepStream SDK Documentation](https://docs.nvidia.com/metropolis/deepstream/dev-guide/)
- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [NVIDIA Jetson Developer Zone](https://developer.nvidia.com/embedded/jetson-developer-kit)
