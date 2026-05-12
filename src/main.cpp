#include <Arduino.h>
#include "driver/i2s.h"

// =====================================================
// CONFIGURACION GENERAL
// =====================================================

#define SAMPLE_RATE     8000
#define RECORD_SECONDS  3
#define TOTAL_SAMPLES   (SAMPLE_RATE * RECORD_SECONDS)

// =====================================================
// MAX98357 - ALTAVOZ
// =====================================================

#define I2S_SPK_PORT    I2S_NUM_1

#define SPK_BCLK        26
#define SPK_LRC         25
#define SPK_DIN         22

// =====================================================
// INMP441 - MICROFONO
// =====================================================

#define I2S_MIC_PORT    I2S_NUM_0

#define MIC_SCK         32
#define MIC_WS          33
#define MIC_SD          34

// =====================================================
// VARIABLES
// =====================================================

int16_t audioGrabado[TOTAL_SAMPLES];
bool mensajeGrabado = false;

// =====================================================
// CONFIGURAR MICROFONO INMP441 - CANAL DERECHO
// =====================================================

void configurarMicrofono() {
  i2s_config_t micConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 128,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t micPins = {
    .bck_io_num = MIC_SCK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD
  };

  esp_err_t err;

  err = i2s_driver_install(I2S_MIC_PORT, &micConfig, 0, NULL);
  if (err != ESP_OK) {
    Serial.print("ERROR instalando driver del microfono: ");
    Serial.println(err);
    while (true) delay(1000);
  }

  err = i2s_set_pin(I2S_MIC_PORT, &micPins);
  if (err != ESP_OK) {
    Serial.print("ERROR configurando pines del microfono: ");
    Serial.println(err);
    while (true) delay(1000);
  }

  i2s_zero_dma_buffer(I2S_MIC_PORT);
}

// =====================================================
// CONFIGURAR ALTAVOZ MAX98357
// =====================================================

void configurarAltavoz() {
  i2s_config_t spkConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 128,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t spkPins = {
    .bck_io_num = SPK_BCLK,
    .ws_io_num = SPK_LRC,
    .data_out_num = SPK_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  esp_err_t err;

  err = i2s_driver_install(I2S_SPK_PORT, &spkConfig, 0, NULL);
  if (err != ESP_OK) {
    Serial.print("ERROR instalando driver del altavoz: ");
    Serial.println(err);
    while (true) delay(1000);
  }

  err = i2s_set_pin(I2S_SPK_PORT, &spkPins);
  if (err != ESP_OK) {
    Serial.print("ERROR configurando pines del altavoz: ");
    Serial.println(err);
    while (true) delay(1000);
  }

  i2s_zero_dma_buffer(I2S_SPK_PORT);
}

// =====================================================
// GRABAR AUDIO
// =====================================================

void grabarAudio() {
  Serial.println();
  Serial.println("Empieza a captar el microfono...");
  Serial.println("Canal usado: DERECHO");
  Serial.println("Habla ahora durante 3 segundos.");

  delay(500);

  int32_t bufferEntrada[128];
  size_t muestrasGrabadas = 0;
  int32_t picoMaximo = 0;

  while (muestrasGrabadas < TOTAL_SAMPLES) {
    size_t bytesLeidos = 0;

    esp_err_t err = i2s_read(
      I2S_MIC_PORT,
      bufferEntrada,
      sizeof(bufferEntrada),
      &bytesLeidos,
      portMAX_DELAY
    );

    if (err != ESP_OK) {
      Serial.print("ERROR leyendo microfono: ");
      Serial.println(err);
      return;
    }

    int muestrasLeidas = bytesLeidos / sizeof(int32_t);

    for (int i = 0; i < muestrasLeidas && muestrasGrabadas < TOTAL_SAMPLES; i++) {
      int32_t muestra32 = bufferEntrada[i];

      int32_t muestra16 = muestra32 >> 14;
      muestra16 = muestra16 * 4;

      if (muestra16 > 32767) muestra16 = 32767;
      if (muestra16 < -32768) muestra16 = -32768;

      audioGrabado[muestrasGrabadas] = (int16_t)muestra16;

      int32_t valorAbs = abs(muestra16);
      if (valorAbs > picoMaximo) {
        picoMaximo = valorAbs;
      }

      muestrasGrabadas++;
    }
  }

  mensajeGrabado = true;

  Serial.println("Mensaje captado correctamente.");
  Serial.print("Muestras grabadas: ");
  Serial.println(muestrasGrabadas);
  Serial.print("Nivel maximo detectado: ");
  Serial.println(picoMaximo);

  if (picoMaximo < 500) {
    Serial.println("AVISO: sigue sin llegar senal suficiente.");
    Serial.println("Revisa especialmente: L/R a 3V3, SD a GPIO34, VDD a 3V3 y GND comun.");
  }

  Serial.println("Reproduciendo mensaje en bucle...");
}

// =====================================================
// REPRODUCIR AUDIO
// =====================================================

void reproducirAudio() {
  const int BLOQUE = 128;
  int16_t bufferSalida[BLOQUE * 2];

  size_t posicion = 0;

  while (posicion < TOTAL_SAMPLES) {
    int muestrasBloque = BLOQUE;

    if (posicion + muestrasBloque > TOTAL_SAMPLES) {
      muestrasBloque = TOTAL_SAMPLES - posicion;
    }

    for (int i = 0; i < muestrasBloque; i++) {
      int16_t muestra = audioGrabado[posicion + i];

      bufferSalida[i * 2] = muestra;
      bufferSalida[i * 2 + 1] = muestra;
    }

    size_t bytesEscritos = 0;

    i2s_write(
      I2S_SPK_PORT,
      bufferSalida,
      muestrasBloque * 2 * sizeof(int16_t),
      &bytesEscritos,
      portMAX_DELAY
    );

    posicion += muestrasBloque;
  }

  delay(250);
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 - INMP441 + MAX98357");
  Serial.println("Grabacion desde microfono y reproduccion en bucle");
  Serial.println("Microfono configurado en CANAL DERECHO");

  configurarMicrofono();
  configurarAltavoz();

  grabarAudio();
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  if (mensajeGrabado) {
    reproducirAudio();
  }
}