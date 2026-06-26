# Simulación Wokwi — Estación Meteorológica IoT (BIU CSE6011)

Esta carpeta contiene un proyecto Wokwi completo que simula el circuito real
(ESP32-S3-DevKitC-1 + BME280 + OLED SSD1306) usando un **chip personalizado**
que actúa como un BME280 real por I²C, ya que Wokwi no incluye este sensor
en su catálogo nativo de componentes.

## Archivos

- `sketch.ino` — firmware (lee el sensor, actualiza el OLED, construye el JSON)
- `diagram.json` — circuito: ESP32-S3 + BME280 simulado + OLED + analizador lógico
- `bme280_custom.chip.c` — lógica del BME280 simulado (responde por I²C en 0x76)
- `bme280_custom.chip.json` — pinout del chip personalizado
- `libraries.txt` — librerías Arduino necesarias

## Cómo importarlo en wokwi.com

1. Ve a **https://wokwi.com/projects/new/esp32-s3** (crea un proyecto ESP32-S3 nuevo).
2. En el panel de archivos (izquierda), reemplaza el contenido de `sketch.ino`,
   `diagram.json` y `libraries.txt` con el de los archivos de esta carpeta
   (copiar y pegar el contenido completo de cada uno).
3. Para el chip personalizado: haz clic en el botón azul **"+"** sobre el
   diagrama → **"Custom Chip"** → lenguaje **C** → nómbralo `bme280_custom`.
   Esto crea `bme280_custom.chip.c` y `bme280_custom.chip.json` en el
   proyecto: reemplaza su contenido con el de los archivos de esta carpeta.
4. Verifica que el chip personalizado aparezca en el diagrama con el `id`
   `bme1` (si Wokwi le asigna otro id automáticamente, ajusta las líneas
   correspondientes en `diagram.json`, o simplemente arrastra las conexiones
   manualmente siguiendo el diagrama: VCC→3V3, GND→GND, SDA→GPIO8, SCL→GPIO9).
5. Presiona ▶ **Start Simulation**. En el Monitor Serie deberías ver el
   sensor detectado en 0x76, la conexión a "Wokwi-GUEST", y cada ~6 segundos
   una lectura nueva (T/H/P) junto con el JSON que se habría enviado.
6. (Opcional) Agrega el **Logic Analyzer** desde el botón "+" si no quedó
   incluido al pegar el diagrama, para visualizar las transacciones I²C en
   tiempo real mientras corre la simulación.

## Por qué no se envía el HTTP POST real

El sketch construye el JSON exactamente igual que el firmware real, pero
**no ejecuta el POST** contra el endpoint de AWS API Gateway de producción.
Esto es intencional: evita insertar lecturas sintéticas (generadas por la
simulación) en la base de datos real de Supabase que alimenta el dashboard
de Grafana. El payload exacto que se habría enviado queda impreso por
consola para verificación.

## Limitación conocida

Wokwi no simula Bluetooth Low Energy, por lo que el módulo de configuración
WiFi por BLE del dispositivo físico no es reproducible aquí. Esta versión
usa la red WiFi virtual "Wokwi-GUEST" (con salida real a internet) en su
lugar.
