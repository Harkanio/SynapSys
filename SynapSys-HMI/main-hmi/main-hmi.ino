#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "SD.h"
#include "SPI.h"

// ================== ⚙️ CONFIGURACIÓN DE HARDWARE ==================
#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

#define SD_SCK        18
#define SD_MISO       19
#define SD_MOSI       23
#define SD_CS         5

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// ================== 🧠 OBJETOS GLOBALES ==================
SPIClass spi_touch(VSPI);
SPIClass spi_sd(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static lv_obj_t *label_hora;
static lv_obj_t *label_fecha;
static lv_obj_t *label_estado;

// ================== 📊 VARIABLES ==================
String hora = "12:45 PM";
String fecha = "01/11/2025";
String estadoInternet = "Desconectado";

// ================== 🧩 FUNCIONES AUXILIARES ==================
extern "C" uint8_t temprature_sens_read(void);
float getInternalTemp() {
  return (temprature_sens_read() - 32) / 1.8;
}

void log_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  Serial.println(buf);
}

// ================== 🖱️ LECTURA DE PANTALLA TÁCTIL ==================
void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    data->point.y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// ================== 🖥️ INTERFAZ PRINCIPAL ==================
void lv_create_main_gui() {
  lv_obj_t *scr = lv_scr_act();

  // Barra superior
  lv_obj_t *barra_estado = lv_obj_create(scr);
  lv_obj_set_size(barra_estado, lv_pct(100), 30);
  lv_obj_align(barra_estado, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_flex_flow(barra_estado, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(barra_estado,
                        LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Estilos
  lv_obj_set_style_bg_color(barra_estado, lv_color_hex(0x303030), 0);
  lv_obj_set_style_border_width(barra_estado, 0, 0);
  lv_obj_set_style_pad_all(barra_estado, 6, 0);

  // Etiquetas
  label_hora = lv_label_create(barra_estado);
  lv_label_set_text(label_hora, hora.c_str());
  lv_obj_set_style_text_color(label_hora, lv_color_white(), 0);

  label_fecha = lv_label_create(barra_estado);
  lv_label_set_text(label_fecha, fecha.c_str());
  lv_obj_set_style_text_color(label_fecha, lv_color_white(), 0);

  label_estado = lv_label_create(barra_estado);
  lv_label_set_text(label_estado, estadoInternet.c_str());
  lv_obj_set_style_text_color(label_estado, lv_color_white(), 0);
}

// ================== 💾 INICIALIZACIÓN DE SD ==================
bool initSD() {
  spi_sd.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  const int MAX_INTENTOS = 5;
  uint32_t freq = 80000000;
  bool sdOk = false;

  for (int intento = 1; intento <= MAX_INTENTOS && !sdOk; intento++) {
    Serial.printf("Intentando inicializar la SD... (Intento %d/%d) Frecuencia: %lu Hz\n",
                  intento, MAX_INTENTOS, freq);

    if (SD.begin(SD_CS, spi_sd, freq)) {
      Serial.println("✅ SD inicializada correctamente");
      sdOk = true;
      break;
    }

    Serial.println("⚠️  Error inicializando la SD");
    delay(1000);

    // Ajuste progresivo de frecuencia
    if (intento == 3) {
      freq = 40000000;
      Serial.println("🔽 Bajando frecuencia SPI a 40 MHz...");
    } else if (intento == 4) {
      freq = 20000000;
      Serial.println("🔽 Bajando frecuencia SPI a 20 MHz...");
    }
  }

  if (!sdOk) {
    Serial.println("❌ No se pudo inicializar la SD después de varios intentos");
    return false;
  }

  // Información de la tarjeta
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("UNKNOWN"); break;
  }

  uint64_t total = SD.totalBytes();
  uint64_t used  = SD.usedBytes();
  Serial.printf("💾 Tamaño total: %llu MB\n", SD.cardSize() / (1024 * 1024));
  Serial.printf("📂 Espacio usado: %llu MB\n", used / (1024 * 1024));
  Serial.printf("📭 Espacio libre: %llu MB\n", (total - used) / (1024 * 1024));

  return true;
}

// ================== 🚀 SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== INICIO DEL SISTEMA =====\n");

  lv_init();
  lv_log_register_print_cb(log_print);

  // Inicializar pantalla táctil
  spi_touch.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(spi_touch);
  touchscreen.setRotation(2);

  // Inicializar TFT
  lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  // Inicializar entrada táctil
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Crear interfaz principal
  lv_create_main_gui();

  // Inicializar SD
  if (!initSD()) return;

  Serial.println("\n✅ Sistema inicializado correctamente\n");
}

// ================== 🔁 LOOP ==================
void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(1000);

  // Actualización de ejemplo
  float temp = getInternalTemp();
  Serial.printf("🌡️  Temperatura interna del CPU: %.2f °C\n", temp);

  // Simulación de actualización de etiquetas
  hora = "12:46 PM";
  fecha = "01/11/2025";
  estadoInternet = "Conectado";

  lv_label_set_text(label_hora, hora.c_str());
  lv_label_set_text(label_fecha, fecha.c_str());
  lv_label_set_text(label_estado, estadoInternet.c_str());
}
