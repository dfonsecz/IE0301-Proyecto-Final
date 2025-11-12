# IE0301 Proyecto Final - Sistema de Vigilancia

El objetivo de este proyecto es desarrollar una aplicación destinada a un sistema de vigilancia basado en visión por computador e inteligencia artificial, cuyo propósito es detectar automóviles.

Como parte de los requisitos, se debe identificar cuándo un vehículo entra a una zona especificada dentro de la imagen, la cual se denomina **Región de Interés (ROI)**. La ubicación y dimensiones de esta región son configurables en los parámetros de la aplicación.

La aplicación determina si se detecta que el **bounding box** de un objeto está dentro de la **ROI**, momento en el cual modifica la imagen para señalar la presencia del objeto en esa zona.

Además, se **monitorea el tiempo** que permanece el objeto dentro de la ROI. Este intervalo de tiempo también es un parámetro de la aplicación, por lo que puede ser modificado.

## Índice

1. [Especificaciones de hardware](#especificaciones-de-hardware)
2. [Especificaciones de software](#especificaciones-de-software)
3. [Estructura del proyecto](#estructura-del-proyecto)
4. [Prerequisitos](#prerequisitos)
5. [Instalación](#instalación)
6. [Cómo utilizar](#cómo-utilizar)
7. [Demostración de uso](#demostración-de-uso)
8. [Licencia](#licencia)
9. [Contacto](#contacto)

## Especificaciones de hardware

- **SoC:** Tegra X1
- **SoM:** NVIDIA Jetson Nano
- **Carrier board:** NVIDIA Jetson Nano developer kit

## Especificaciones de software

- **BSP:** Jetpack 4.6.4
- **Multimedia framework:** GStreamer 1.14.5
- **Video analytics / AI framework:** NVIDIA DeepStream SDK 6.0

## Estructura del proyecto

## Prerequisitos

## Instalación

En primer lugar, clone este repositorio en su dispositivo.

```
git clone https://github.com/dfonsecz/IE0301-Proyecto-Final.git
```

## Cómo utilizar

## Demostración de uso

## Licencia

Este proyecto utiliza una licencia del MIT, puede encontrar los detalles en [LICENSE](https://github.com/dfonsecz/IE0301-Proyecto-Final/blob/main/LICENSE).

## Contacto