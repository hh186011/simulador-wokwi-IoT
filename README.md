# Simulación Wokwi — Estación Meteorológica IoT (BIU CSE6011)

Simulación del Caso de Estudio "Estación Meteorológica IoT" desarrollado para
la asignatura **Advanced Computer Structures (CSE6011)**, Maestría en
Ingeniería en Informática con aplicación en IA, Broward International
University. Reproduce el circuito real (ESP32-S3-DevKitC-1 + sensor BME280 +
pantalla OLED SSD1306) en [Wokwi](https://wokwi.com), incluyendo un **chip
personalizado** que emula el sensor BME280 por I²C, ya que este sensor no
está disponible como componente nativo en el catálogo de Wokwi.

▶️ **Ejecutar la simulación en vivo:** https://wokwi.com/projects/467942425631185921

## Contexto del proyecto

El proyecto completo de la Estación Meteorológica IoT consta de 6 capas
(dispositivo → comunicación → API Gateway → procesamiento serverless →
persistencia → visualización). Este repositorio cubre únicamente la capa de
**simulación/validación lógica** del firmware del dispositivo. El resto del
sistema (backend Lambda, dashboard, servidor MCP) está documentado en el
informe técnico del proyecto y en el repositorio
[lambda-iot-aws-BIU](https://github.com/hh186011/lambda-iot-aws-BIU).

## Por qué un chip personalizado

Wokwi no incluye el BME280 en su catálogo nativo de componentes (solo el
BMP280 oficial, que no mide humedad). Para que la librería **Adafruit_BME280**
original —sin ninguna modificación— funcione igual que con el sensor físico,
se implementó un chip personalizado mediante la
[Custom Chips API](https://docs.wokwi.com/chips-api/getting-started) de Wokwi:
actúa como un esclavo I²C real en la dirección `0x76`, respondiendo con el
mismo mapa de registros que el sensor físico (chip ID, bloque de calibración
de fábrica, registros de datos crudos).

Los valores crudos fueron precalculados invirtiendo numéricamente la fórmula
oficial de compensación de Bosch (Python + búsqueda de raíces), generando una
tabla de 24 muestras que recorre un ciclo diario plausible para Santiago de
Chile (temperatura ~6,5–20,5 °C, humedad relativa 58–86 %, presión
1009–1015 hPa).

## Estructura del repositorio

| Archivo | Descripción |
|---|---|
| `sketch.ino` | Firmware: lee el sensor, actualiza el OLED y construye el payload JSON |
| `diagram.json` | Circuito: ESP32-S3 + BME280 simulado + OLED + analizador lógico |
| `bme280_custom.chip.c` | Lógica del BME280 simulado (esclavo I²C en `0x76`) |
| `bme280_custom.chip.json` | Pinout del chip personalizado (VCC, GND, SCL, SDA, CSB, SDO) |
| `libraries.txt` | Librerías Arduino requeridas |

## Cómo correrlo

**Opción A — Wokwi (recomendada, sin instalación):**
1. Abre el link de simulación en vivo (arriba) y presiona ▶ **Start Simulation**.

**Opción B — Wokwi CLI / VS Code (desde este repo clonado):**
1. Clona el repositorio.
2. Instala la [extensión Wokwi para VS Code](https://docs.wokwi.com/vscode/getting-started) o el [Wokwi CLI](https://docs.wokwi.com/wokwi-ci/getting-started).
3. Abre la carpeta del repo y ejecuta la simulación (F1 → "Wokwi: Start Simulator").

En el Monitor Serie deberías ver el sensor detectado en `0x76`, la conexión a
la red virtual `Wokwi-GUEST`, y cada ~6 segundos una lectura nueva
(temperatura/humedad/presión) junto con el JSON que se habría enviado al
backend.

## Por qué no se envía el HTTP POST real

El sketch construye el JSON exactamente igual que el firmware real, pero
**no ejecuta el POST** contra el endpoint de AWS API Gateway de producción.
Esto es intencional: evita insertar lecturas sintéticas (generadas por la
simulación) en la base de datos real de Supabase que alimenta el dashboard
de Grafana. El payload exacto que se habría enviado queda impreso por
consola para verificación.

## Limitaciones conocidas

- **Bluetooth Low Energy:** Wokwi no simula BLE, por lo que el módulo de
  configuración WiFi por BLE del dispositivo físico no es reproducible aquí.
  Esta versión usa la red WiFi virtual `Wokwi-GUEST` (con salida real a
  internet) en su lugar.
- **Calibración del sensor:** los coeficientes de calibración son plausibles
  pero sintéticos (no provienen de un chip físico específico); el objetivo es
  validar la lógica del firmware, no replicar un sensor individual exacto.

## Autor

Héctor Marcelo Hernández Larrañaga — Maestría en Ingeniería en Informática
con aplicación en Inteligencia Artificial, Broward International University.
Docente: Cristian Gabriel Zambrano Vega (CSE6011).
