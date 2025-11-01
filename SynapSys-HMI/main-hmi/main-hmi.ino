#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "SD.h"
#include "SPI.h"

// ================== TOUCHSCREEN PINS ==================
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ================== SD PINS ==================
#define SCK_SD  18
#define MISO_SD 19
#define MOSI_SD 23
#define CS_SD   5

SPIClass touchscreenSPI = SPIClass(VSPI);
SPIClass spi_sd = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// ================== LVGL CONFIG ==================
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// ================== VARIABLES ==================
int btn_count = 0;
static lv_obj_t * slider_label;

// ================== TEMPERATURA CPU ==================
extern "C" uint8_t temprature_sens_read(void);

float getInternalTemp() {
  // Convierte de Fahrenheit a Celsius
  return (temprature_sens_read() - 32) / 1.8;
}

// ================== FUNCIONES ==================
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
}

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Callback del botón
static void event_handler_btn(lv_event_t * e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    btn_count++;
    LV_LOG_USER("Button clicked %d", btn_count);
  }
}

// Callback del slider
static void slider_event_callback(lv_event_t * e) {
  lv_obj_t * slider = (lv_obj_t*) lv_event_get_target(e);
  char buf[8];
  lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
  lv_label_set_text(slider_label, buf);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

// Crear GUI básica
void lv_create_main_gui(void) {
  lv_obj_t * label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello, World!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -90);

  // Botón
  lv_obj_t * btn = lv_button_create(lv_scr_act());
  lv_obj_add_event_cb(btn, event_handler_btn, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, -40);
  lv_obj_t * btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Click me");
  lv_obj_center(btn_label);

  // Slider
  lv_obj_t * slider = lv_slider_create(lv_scr_act());
  lv_slider_set_range(slider, 0, 100);
  lv_obj_add_event_cb(slider, slider_event_callback, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 40);

  // Label del slider
  slider_label = lv_label_create(lv_scr_act());
  lv_label_set_text(slider_label, "0%");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}


// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  lv_init();
  lv_log_register_print_cb(log_print);

  // Inicializar pantalla táctil
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  // Inicializar TFT
  lv_display_t * disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  // Inicializar input device
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Crear GUI
  lv_create_main_gui();

  // ================== INICIALIZAR SD ==================
  spi_sd.begin(SCK_SD, MISO_SD, MOSI_SD, CS_SD);

  const int MAX_INTENTOS = 5;
  int intentos = 0;
  bool sdOk = false;
  uint32_t freq = 80000000; // Frecuencia inicial (80 MHz)

  while (intentos < MAX_INTENTOS && !sdOk) {
    Serial.printf("Intentando inicializar la SD... (Intento %d/%d) Frecuencia: %lu Hz\n", intentos + 1, MAX_INTENTOS, freq);
    if (SD.begin(CS_SD, spi_sd, freq)) {
      sdOk = true;
      Serial.println("✅ SD inicializada correctamente");
      break;
    } else {
      Serial.println("⚠️  Error inicializando la SD");
      delay(1000);
    }

    intentos++;

    // Reducir frecuencia si fallan más de 3 intentos
    if (intentos == 3) {
      freq = 40000000; // Baja a 40 MHz
      Serial.println("🔽 Bajando frecuencia SPI a 40 MHz...");
    } else if (intentos == 4) {
      freq = 20000000; // Baja a 20 MHz
      Serial.println("🔽 Bajando frecuencia SPI a 20 MHz...");
    }
  }

  if (!sdOk) {
    Serial.println("❌ No se pudo inicializar la SD después de varios intentos");
    return;
  }

  // Mostrar información de la SD
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  // Tamaño total de la SD
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("💾 Tamaño total: %llu MB\n", cardSize);

  // Espacio disponible
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  uint64_t freeBytes = totalBytes - usedBytes;

  Serial.printf("📂 Espacio usado: %llu MB\n", usedBytes / (1024 * 1024));
  Serial.printf("📭 Espacio libre: %llu MB\n", freeBytes / (1024 * 1024));

  Serial.println("\n✅ Sistema inicializado correctamente\n");
}

// ================== LOOP ==================
void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(1000);

  // Leer temperatura cada segundo
  float temp = getInternalTemp();
  Serial.printf("🌡️  Temperatura interna del CPU: %.2f °C\n", temp);
}
