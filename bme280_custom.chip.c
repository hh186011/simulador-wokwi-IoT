/*
 * BME280 simulado (Custom Chip API de Wokwi) - Proyecto BIU CSE6011
 * Estacion Meteorologica IoT - Hector Hernandez
 *
 * Este chip actua como un esclavo I2C real en la direccion 0x76 y responde
 * a los mismos registros que un BME280 fisico (chip id, calibracion,
 * control y datos crudos), de forma que la libreria Adafruit_BME280
 * original (sin modificar) funcione exactamente igual que con el sensor
 * real. Los valores crudos fueron precalculados en Python invirtiendo la
 * formula oficial de compensacion de Bosch, por lo que al decodificarlos
 * la libreria entrega temperatura/humedad/presion realistas para
 * Santiago, variando a lo largo de 24 muestras (ciclo diario simulado).
 *
 * No implementa el analizador de I2C en si (eso lo hace el componente
 * "Logic Analyzer" conectado en paralelo al bus en diagram.json); este
 * chip es el dispositivo esclavo que hace que haya trafico real que
 * capturar.
 */

#include "wokwi-api.h"
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------
// Calibracion (registros 0x88-0xA1 y 0xE1-0xE7), generada en Python a
// partir de coeficientes plausibles de fabrica (ver gen_calib.py)
// ---------------------------------------------------------------------
static const uint8_t CALIB_88[26] = {
  112,107,  67,103,  24,252,           // T1, T2, T3
  125,142,  67,214, 208,11, 39,11,     // P1, P2, P3, P4
  140,0,   249,255, 140,60, 56,199,    // P5, P6, P7, P8
  112,23,                              // P9
  0,                                   // 0xA0 reservado
  75                                   // H1
};

static const uint8_t CALIB_E1[7] = { 116, 1, 0, 17, 42, 3, 30 };

// ---------------------------------------------------------------------
// Tabla de 24 muestras {adc_T, adc_P, adc_H} -> ciclo diario simulado
// (generadas invirtiendo numericamente la formula de compensacion de
// Bosch para que la libreria Adafruit_BME280 decodifique valores
// realistas de Santiago: ~6.5 a 20.5 C, 58-86% HR, 1009-1015 hPa)
// ---------------------------------------------------------------------
static const uint32_t SAMPLES[24][3] = {
  {467223u, 396819u, 32768u},
  {463687u, 394881u, 33197u},
  {461464u, 393568u, 33469u},
  {460706u, 393102u, 33562u},
  {461464u, 393568u, 33469u},
  {463687u, 394881u, 33197u},
  {467223u, 396819u, 32768u},
  {471833u, 399066u, 32211u},
  {477204u, 401286u, 31569u},
  {482970u, 403199u, 30887u},
  {488738u, 404629u, 30210u},
  {494115u, 405532u, 29586u},
  {498735u, 405978u, 29055u},
  {502280u, 406116u, 28650u},
  {504510u, 406112u, 28396u},
  {505270u, 406095u, 28310u},
  {504510u, 406112u, 28396u},
  {502280u, 406116u, 28650u},
  {498735u, 405978u, 29055u},
  {494115u, 405532u, 29586u},
  {488738u, 404629u, 30210u},
  {482970u, 403199u, 30887u},
  {477204u, 401286u, 31569u},
  {471833u, 399066u, 32211u},
};
#define NUM_SAMPLES 24

// ---------------------------------------------------------------------
// Estado del chip
// ---------------------------------------------------------------------
typedef struct {
  pin_t pin_scl;
  pin_t pin_sda;
  i2c_dev_t i2c;
  timer_t sample_timer;

  uint16_t reg_ptr;       // puntero de registro actual
  bool     expect_reg;    // true = el siguiente byte escrito es el puntero
  uint8_t  ctrl_hum;
  uint8_t  ctrl_meas;
  uint8_t  config_reg;
  uint8_t  data_f7[8];    // press(3) temp(3) hum(2), igual que el real
  uint32_t sample_idx;
} chip_state_t;

static chip_state_t state;

static void encode_sample(uint32_t idx) {
  uint32_t adc_T = SAMPLES[idx][0];
  uint32_t adc_P = SAMPLES[idx][1];
  uint32_t adc_H = SAMPLES[idx][2];

  // Presion: 20 bits en press_msb,press_lsb,press_xlsb(bits 7:4)
  state.data_f7[0] = (uint8_t)((adc_P >> 12) & 0xFF);
  state.data_f7[1] = (uint8_t)((adc_P >> 4) & 0xFF);
  state.data_f7[2] = (uint8_t)((adc_P << 4) & 0xF0);

  // Temperatura: 20 bits en temp_msb,temp_lsb,temp_xlsb(bits 7:4)
  state.data_f7[3] = (uint8_t)((adc_T >> 12) & 0xFF);
  state.data_f7[4] = (uint8_t)((adc_T >> 4) & 0xFF);
  state.data_f7[5] = (uint8_t)((adc_T << 4) & 0xF0);

  // Humedad: 16 bits en hum_msb,hum_lsb
  state.data_f7[6] = (uint8_t)((adc_H >> 8) & 0xFF);
  state.data_f7[7] = (uint8_t)(adc_H & 0xFF);
}

static void on_sample_timer(void *user_data) {
  (void)user_data;
  state.sample_idx = (state.sample_idx + 1) % NUM_SAMPLES;
  encode_sample(state.sample_idx);
  printf("[BME280 sim] muestra %lu -> registros actualizados\n",
         (unsigned long)state.sample_idx);
}

static bool on_i2c_connect(void *user_data, uint32_t address, bool read) {
  (void)user_data;
  (void)address;
  if (!read) {
    state.expect_reg = true; // la proxima escritura sera el puntero de registro
  }
  return true; // ACK siempre (solo escuchamos en 0x76 segun i2c_config_t.address)
}

static uint8_t on_i2c_read(void *user_data) {
  (void)user_data;
  uint8_t value = 0x00;
  uint16_t reg = state.reg_ptr;

  if (reg == 0xD0) {
    value = 0x60; // chip id BME280
  } else if (reg >= 0x88 && reg <= 0xA1) {
    value = CALIB_88[reg - 0x88];
  } else if (reg >= 0xE1 && reg <= 0xE7) {
    value = CALIB_E1[reg - 0xE1];
  } else if (reg == 0xF2) {
    value = state.ctrl_hum;
  } else if (reg == 0xF3) {
    value = 0x00; // status: medicion no en curso, NVM listo
  } else if (reg == 0xF4) {
    value = state.ctrl_meas;
  } else if (reg == 0xF5) {
    value = state.config_reg;
  } else if (reg >= 0xF7 && reg <= 0xFE) {
    value = state.data_f7[reg - 0xF7];
  }

  state.reg_ptr++;
  return value;
}

static bool on_i2c_write(void *user_data, uint8_t data) {
  (void)user_data;
  if (state.expect_reg) {
    state.reg_ptr = data;
    state.expect_reg = false;
    return true;
  }

  switch (state.reg_ptr) {
    case 0xF2: state.ctrl_hum   = data; break;
    case 0xF4: state.ctrl_meas  = data; break;
    case 0xF5: state.config_reg = data; break;
    case 0xE0: /* soft reset, se ignora en la simulacion */ break;
    default: break;
  }
  state.reg_ptr++;
  return true;
}

void chip_init(void) {
  memset(&state, 0, sizeof(state));

  state.pin_scl = pin_init("SCL", INPUT_PULLUP);
  state.pin_sda = pin_init("SDA", INPUT_PULLUP);
  pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  pin_init("CSB", INPUT_PULLUP); // HIGH = modo I2C (no se simula SPI)
  pin_init("SDO", INPUT_PULLDOWN); // LOW = direccion 0x76

  state.ctrl_hum   = 0x01;
  state.ctrl_meas  = 0x27;
  state.config_reg = 0x00;
  state.sample_idx = 0;
  encode_sample(0);

  i2c_config_t i2c_config = {
    .user_data = NULL,
    .address = 0x76,
    .scl = state.pin_scl,
    .sda = state.pin_sda,
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = NULL,
  };
  state.i2c = i2c_init(&i2c_config);

  timer_config_t timer_config = {
    .user_data = NULL,
    .callback = on_sample_timer,
  };
  state.sample_timer = timer_init(&timer_config);
  timer_start(state.sample_timer, 4000000, true); // cada 4s (sim), avanza de muestra

  printf("[BME280 sim] chip inicializado, direccion I2C 0x76\n");
}
