# SynapSys

![Estado](https://img.shields.io/badge/Estado-En%20Desarrollo-yellow)
![Arduino](https://img.shields.io/badge/IDE-Arduino-brightgreen)
![VSCode](https://img.shields.io/badge/IDE-VSCode-blue)
![Licencia](https://img.shields.io/badge/Licencia-MIT-lightgrey)

---

## Descripción General

**SynapSys** es un proyecto en desarrollo orientado a la automatización y control de una estación industrial universitaria simulada. Integra **procesamiento de datos, control de actuadores, reconocimiento de audio e imagen**, y comunicación entre módulos para simular un ambiente industrial funcional.

La estación incluye:  

- **Brazo robot Mitsubishi** que transporta piezas desde la banda transportadora hasta una fresadora.  
- **Fresadora** que transforma las piezas en figuras específicas.  
- **Sección de reconocimiento de imagen** para identificar la forma final de las piezas.  

El proyecto busca un **prototipo funcional**, educativo y demostrativo, usando microcontroladores ESP32-S3 y periféricos de comunicación industrial.

---

## Objetivo

Automatizar una estación industrial universitaria donde:  

1. Una pieza llega por banda transportadora al brazo robot Mitsubishi.  
2. El robot detecta la pieza y la transporta hacia la fresadora, que transforma la pieza en una figura específica.  
3. La pieza es movida a una sección de reconocimiento mediante procesamiento de imagen.  
4. Toda la información y control puede ser monitoreada y gestionada a través de la HMI y mensajes de Telegram.

---

## Arquitectura del Proyecto

### Diagrama de flujo general (ASCII)

       +------------------+
       | PLC Allen-Bradley|
       |      1400        |
       +--------+---------+
                |
             RS232
                |
           +----v-----+
           |  Central |
           |  ESP32-S3|
           +----+-----+
     UART/I2C/SPI|Wi-Fi/Telegram
     -------------+-----------------
    |               |               |
    +----v----+ +----v----+ +----v----+
    | HMI     | | Audio   | | Imagen  |
    | ESP32   | | ESP32-S3| | ESP32-S3|
    | TFT 2.8"| | INMP441 | | OV3660  |
    +---------+ +---------+ +---------+

- El **PLC** se comunica con la **Central** mediante **RS232** usando un **MAX3535** para adaptación de niveles.  
- La **Central** coordina todos los módulos y gestiona la comunicación remota vía **Telegram**.  
- La **HMI** funciona como servidor local y pantalla táctil.  
- El **Audio** procesa comandos de voz y aplica lógica difusa.  
- El **Imagen** reconoce figuras usando **TensorFlow Lite Micro**.  

---

### Protocolos de comunicación

- **RS232** entre PLC y Central (vía MAX3535)  
- **UART, I2C y SPI** entre módulos (según necesidad)  
- **Wi-Fi / Telegram** para control remoto y notificaciones  

---

## Módulos

### Central
- Comunicación con **PLC** y nodos  
- Mensajería **Telegram** para monitorización y control  
- **Hardware**: ESP32-S3  

### HMI
- Pantalla táctil **TFT CYD 2.8"**  
- Servidor local para mostrar datos y bases de datos en **SD** y **PA con WebStocks**  
- **Hardware**: ESP32 con pantalla TFT CYD 2.8" (ESP32-2432S028R)  

### Audio
- Procesamiento de comandos de voz simples: “Sinapsis: Encender”, “Sinapsis: Apagar”  
- Lógica difusa para interpretación de comandos  
- **Hardware**: ESP32-S3 + micrófono INMP441  

### Imagen
- Reconocimiento de formas: estrella, triángulo, rectángulo, círculo  
- Clasificación mediante **TensorFlow Lite Micro**  
- Comunicación con Central para resultados  
- **Hardware**: ESP32-S3 + cámara OV3660  

---

## Tecnologías y Herramientas

- **Microcontroladores**: ESP32 y ESP32-S3  
- **Pantalla**: TFT CYD 2.8" (entrada táctil)  
- **Micrófono**: INMP441  
- **Cámara**: OV3660  
- **Software**: Arduino IDE, Visual Studio Code con ESP-IDF  
- **Bibliotecas**: TensorFlow Lite Micro  
- **Protocolos**: RS232, UART, I2C, SPI, Wi-Fi  
- **Otros**: MAX3535 para adaptación de niveles RS232  

---

## Estado del Proyecto

Actualmente en desarrollo:  

- Comunicación Central ↔ PLC implementada y probada  
- Pruebas iniciales de módulos de **Audio** e **Imagen**  
- HMI en desarrollo con servidor local y pantalla táctil  
- Integración general pendiente  

---

## Contribuciones

1. Haz un fork del repositorio  
2. Crea tu branch (`git checkout -b feature/nueva-funcion`)  
3. Haz commit de tus cambios (`git commit -am 'Agrega nueva función'`)  
4. Haz push a la rama (`git push origin feature/nueva-funcion`)  
5. Abre un Pull Request  

---

## Licencia

Este proyecto está bajo la licencia **MIT**.

