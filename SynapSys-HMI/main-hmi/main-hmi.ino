#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "SD.h"
#include "SPI.h"

// ================== CONFIGURACION DE HARDWARE ==================
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

// ================== OBJETOS GLOBALES ==================
SPIClass spi_touch(VSPI);
SPIClass spi_sd(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static lv_obj_t *label_hora;
static lv_obj_t *label_fecha;
static lv_obj_t *label_estado;
static lv_obj_t *active_tab = NULL;
static lv_obj_t *screen_container = NULL;

// ================== VARIABLES ==================
String hora = "12:45 PM";
String fecha = "01/11/2025";
String estadoInternet = "Desconectado";
// ================== VARIABLES ==================
unsigned long lastSDCheck = 0;
const unsigned long SD_CHECK_INTERVAL = 5000; // Revisar cada 5 segundos

bool sdInserted = false; // Estado actual de la SD

// ================== PANTALLA ESTADO ==================
static lv_obj_t *label_sd_status;
static lv_obj_t *bar_sd;

  uint64_t totalBytes;
  uint64_t usedBytes;
  uint64_t freeBytes;


// ================== FUNCIONES AUXILIARES ==================
extern "C" uint8_t temprature_sens_read(void);
float getInternalTemp() {
  return (temprature_sens_read() - 32) / 1.8;
}

void log_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  Serial.println(buf);
}

// ================== LECTURA DE PANTALLA TACTIL ==================
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

// ================== FUNCIONES DE ESTILO ==================
static lv_style_t style_tab_normal;
static lv_style_t style_tab_active;

void style_init_tabs() {
  lv_style_init(&style_tab_normal);
  lv_style_set_bg_color(&style_tab_normal, lv_color_hex(0xE8E8E8));
  lv_style_set_text_color(&style_tab_normal, lv_color_hex(0x202020));
  lv_style_set_border_color(&style_tab_normal, lv_color_hex(0xB0B0B0));
  lv_style_set_border_width(&style_tab_normal, 1);
  lv_style_set_radius(&style_tab_normal, 4);
  lv_style_set_shadow_width(&style_tab_normal, 0);
  lv_style_set_pad_all(&style_tab_normal, 4);

  lv_style_init(&style_tab_active);
  lv_style_set_bg_color(&style_tab_active, lv_color_hex(0x0078D7));
  lv_style_set_text_color(&style_tab_active, lv_color_white());
  lv_style_set_border_color(&style_tab_active, lv_color_hex(0x005A9E));
  lv_style_set_border_width(&style_tab_active, 1);
  lv_style_set_radius(&style_tab_active, 4);
  lv_style_set_shadow_width(&style_tab_active, 0);
  lv_style_set_pad_all(&style_tab_active, 4);
}

// ================== FUNCIONES DE PANTALLAS ==================
void create_home_screen(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Pantalla HOME");
    lv_obj_center(lbl);
}

void create_graficas_screen(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Pantalla GRAFICAS");
    lv_obj_center(lbl);
}

void create_nodos_screen(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Pantalla NODOS");
    lv_obj_center(lbl);
}

void create_estado_screen(lv_obj_t *parent) {
    lv_obj_clean(parent);

    int screen_height = SCREEN_HEIGHT / 3; // Ocupa 1/3 de la pantalla
    int section_height = screen_height / 2 - 5; // Cada sección (WiFi/SD) más compacta

    // ================= SECCION WIFI =================
    lv_obj_t *wifi_cont = lv_obj_create(parent);
    lv_obj_set_size(wifi_cont, LV_PCT(90), section_height);
    lv_obj_set_style_bg_color(wifi_cont, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_radius(wifi_cont, 4, 0);
    lv_obj_set_style_border_width(wifi_cont, 1, 0);
    lv_obj_set_style_border_color(wifi_cont, lv_color_hex(0xB0B0B0), 0);
    lv_obj_set_flex_flow(wifi_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(wifi_cont, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *lbl_wifi_title = lv_label_create(wifi_cont);
    lv_label_set_text(lbl_wifi_title, "WiFi");
    lv_obj_set_style_text_font(lbl_wifi_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_wifi_title, lv_color_black(), 0);

    lv_obj_t *lbl_wifi_status = lv_label_create(wifi_cont);
    lv_label_set_text(lbl_wifi_status, "Estado: Conectado");
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_wifi_status, lv_color_black(), 0);

    // ================= SECCION SD =================
    lv_obj_t *sd_cont = lv_obj_create(parent);
    lv_obj_set_size(sd_cont, LV_PCT(90), section_height);
    lv_obj_set_style_bg_color(sd_cont, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_radius(sd_cont, 4, 0);
    lv_obj_set_style_border_width(sd_cont, 1, 0);
    lv_obj_set_style_border_color(sd_cont, lv_color_hex(0xB0B0B0), 0);
    lv_obj_set_flex_flow(sd_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sd_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(sd_cont, LV_ALIGN_TOP_MID, 0, section_height + 15);

    lv_obj_t *lbl_sd_title = lv_label_create(sd_cont);
    lv_label_set_text(lbl_sd_title, "Memoria SD");
    lv_obj_set_style_text_font(lbl_sd_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_sd_title, lv_color_black(), 0);

    bar_sd = lv_bar_create(sd_cont);
    lv_bar_set_range(bar_sd, 0, 100);
    lv_bar_set_value(bar_sd, 0, LV_ANIM_OFF);
    lv_obj_set_size(bar_sd, LV_PCT(80), 12); // Barra más pequeña
    lv_obj_set_style_pad_all(bar_sd, 0, 0);

    label_sd_status = lv_label_create(sd_cont);
    lv_label_set_text(label_sd_status, "DESCONECTADA");
    lv_obj_set_style_text_font(label_sd_status, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label_sd_status, lv_palette_main(LV_PALETTE_RED), 0);
}


void create_config_screen(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Pantalla CONFIG");
    lv_obj_center(lbl);
}


// ================== CALLBACK DE PESTAÑAS =================
void tab_event_cb(lv_event_t *e) {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

    // Restaurar estilo de la pestaña anterior (si existe)
    if (active_tab && active_tab != btn) {
        lv_obj_remove_style(active_tab, &style_tab_active, 0);
        lv_obj_add_style(active_tab, &style_tab_normal, 0);
    }

    // Aplicar estilo activo a la nueva pestaña
    lv_obj_remove_style(btn, &style_tab_normal, 0);
    lv_obj_add_style(btn, &style_tab_active, 0);
    active_tab = btn;

    // Limpiar contenedor de pantalla
    lv_obj_clean(screen_container);

    // Crear la pantalla correspondiente
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char* tab_name = lv_label_get_text(label);

    if (strcmp(tab_name, "Home") == 0) create_home_screen(screen_container);
    else if (strcmp(tab_name, "Graficas") == 0) create_graficas_screen(screen_container);
    else if (strcmp(tab_name, "Nodos") == 0) create_nodos_screen(screen_container);
    else if (strcmp(tab_name, "Estado") == 0) create_estado_screen(screen_container);
    else if (strcmp(tab_name, "Config") == 0) create_config_screen(screen_container);

    Serial.printf("Pestaña seleccionada: %s\n", tab_name);
}

// ================== CREACION DE INTERFAZ =================
void lv_create_main_gui() {
    lv_obj_t *scr = lv_scr_act();

    // ================= BARRA SUPERIOR =================
    lv_obj_t *barra_estado = lv_obj_create(scr);
    lv_obj_set_size(barra_estado, LV_HOR_RES, 28);
    lv_obj_align(barra_estado, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(barra_estado, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(barra_estado, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(barra_estado, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_width(barra_estado, 0, 0);
    lv_obj_set_style_pad_all(barra_estado, 6, 0);
    lv_obj_set_style_radius(barra_estado, 0, 0);

    label_hora = lv_label_create(barra_estado);
    lv_label_set_text(label_hora, hora.c_str());
    lv_obj_set_style_text_color(label_hora, lv_color_white(), 0);

    label_fecha = lv_label_create(barra_estado);
    lv_label_set_text(label_fecha, fecha.c_str());
    lv_obj_set_style_text_color(label_fecha, lv_color_white(), 0);

    label_estado = lv_label_create(barra_estado);
    lv_label_set_text(label_estado, estadoInternet.c_str());
    lv_obj_set_style_text_color(label_estado, lv_color_white(), 0);

    // ================= CONTENEDOR DE PESTAÑAS =================
    lv_obj_t *tab_container = lv_obj_create(scr);
    lv_obj_set_size(tab_container, LV_HOR_RES, 35);
    lv_obj_align(tab_container, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_flex_flow(tab_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab_container, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(tab_container, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_width(tab_container, 0, 0);
    lv_obj_set_style_pad_all(tab_container, 3, 0);
    lv_obj_set_style_radius(tab_container, 0, 0);

    const char *nombres[] = {"Home", "Graficas", "Nodos", "Estado", "Config"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_btn_create(tab_container);
        lv_obj_set_size(btn, 65, 26);
        lv_obj_add_event_cb(btn, tab_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, nombres[i]);
        lv_obj_center(lbl);

        if (i == 0) {
            lv_obj_add_style(btn, &style_tab_active, 0);
            active_tab = btn;
        } else {
            lv_obj_add_style(btn, &style_tab_normal, 0);
        }
    }

    // ================= CONTENEDOR DE PANTALLA PRINCIPAL =================
int top_offset = 28 + 35; // Barra superior + barra de pestañas
screen_container = lv_obj_create(scr);

// Ocupa TODO el ancho y alto restante
lv_obj_set_size(screen_container, LV_PCT(100), LV_PCT(100)-26); //Con 26 sirve

// Posición: debajo de barras
lv_obj_set_pos(screen_container, 1, top_offset);

// Fondo blanco y sin bordes
lv_obj_set_style_bg_color(screen_container, lv_color_white(), 0);
lv_obj_set_style_radius(screen_container, 0, 0);
lv_obj_set_style_border_width(screen_container, 0, 0);

// Layout para centrar contenido automáticamente
lv_obj_set_layout(screen_container, LV_LAYOUT_FLEX);
lv_obj_set_flex_flow(screen_container, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(screen_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

// ================= PANTALLA INICIAL =================
create_home_screen(screen_container);

}

// ================== FUNCIONES AUXILIARES ==================
void checkSerialUpdate() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim(); // Quita espacios al inicio y fin

    if (line.startsWith("H:")) {
      hora = line.substring(2);
      hora.trim();
      lv_label_set_text(label_hora, hora.c_str());
      Serial.printf("Hora actualizada: %s\n", hora.c_str());
    }
    else if (line.startsWith("F:")) {
      fecha = line.substring(2);
      fecha.trim();
      lv_label_set_text(label_fecha, fecha.c_str());
      Serial.printf("Fecha actualizada: %s\n", fecha.c_str());
    }
    else if (line.startsWith("EI:")) {
      estadoInternet = line.substring(3);
      estadoInternet.trim();
      lv_label_set_text(label_estado, estadoInternet.c_str());
      Serial.printf("Estado Internet actualizado: %s\n", estadoInternet.c_str());
    }
  }
}






// ================== INICIALIZACION DE SD ==================
bool initSD() {
  spi_sd.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  const int MAX_INTENTOS = 5;
  uint32_t freq = 80000000;
  bool sdOk = false;

  for (int intento = 1; intento <= MAX_INTENTOS && !sdOk; intento++) {
    Serial.printf("Intentando inicializar la SD... (Intento %d/%d) Frecuencia: %lu Hz\n",
                  intento, MAX_INTENTOS, freq);

    if (SD.begin(SD_CS, spi_sd, freq)) {
      Serial.println("SD inicializada correctamente");
      sdOk = true;
      sdInserted = true;
      break;
    }
    Serial.println("Error inicializando la SD");
    delay(1000);
    if (intento == 3) freq = 40000000;
    else if (intento == 4) freq = 20000000;
  }

  if (!sdOk) {
    Serial.println("No se pudo inicializar la SD");
    return false;
  }

  uint8_t cardType = SD.cardType();
  Serial.print("Tipo de tarjeta SD: ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC"); break;
    default:        Serial.println("DESCONOCIDO"); break;
  }

  // Tamaño total de la SD
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("💾 Tamaño total: %llu MB\n", cardSize);

  // Espacio disponible
  totalBytes = SD.totalBytes();
  usedBytes = SD.usedBytes();
  freeBytes = totalBytes - usedBytes;

  Serial.printf("📂 Espacio usado: %llu MB\n", usedBytes / (1024 * 1024));
  Serial.printf("📭 Espacio libre: %llu MB\n", freeBytes / (1024 * 1024));

  return true;
}

void checkSDStatus() {
  if (millis() - lastSDCheck >= SD_CHECK_INTERVAL) {
    lastSDCheck = millis();

    if (SD.begin(SD_CS, spi_sd, 40000000)) {
      if (!sdInserted) {
        Serial.println("SD detectada e inicializada correctamente");
        sdInserted = true;
      }
      SD.end();
    } else {
      if (sdInserted) {
        Serial.println("SD retirada o no detectada");
        sdInserted = false;
      }
    }
  }
}

void updateSDDisplay() {
    if (!label_sd_status || !bar_sd) return; // Salir si no existe
    static lv_obj_t *bar_label = nullptr;
    char buf[30];

    if (!bar_label) {
        bar_label = lv_label_create(bar_sd);
        lv_obj_center(bar_label);
    }

    if (sdInserted) {
        lv_label_set_text(label_sd_status, "CONECTADA");
        lv_obj_set_style_text_color(label_sd_status, lv_palette_main(LV_PALETTE_GREEN), 0);

        int usedPercent = (usedBytes * 100) / totalBytes;
        lv_bar_set_value(bar_sd, usedPercent, LV_ANIM_OFF);
        

        sprintf(buf, "%llu / %llu MB", usedBytes / (1024 * 1024), totalBytes / (1024 * 1024));
        
        lv_label_set_text(bar_label, buf);
        lv_obj_set_style_text_color(bar_label, lv_color_hex(0x202020), 0); //
    } else {
        lv_label_set_text(label_sd_status, "DESCONECTADA");
        lv_obj_set_style_text_color(label_sd_status, lv_palette_main(LV_PALETTE_RED), 0);
        lv_bar_set_value(bar_sd, 0, LV_ANIM_OFF);

        sprintf(buf, "? / ? MB");
        lv_label_set_text(bar_label, buf);              // <--- actualiza el texto de la barra
        lv_obj_set_style_text_color(bar_label, lv_color_hex(0x202020), 0); // color gris
    }
}




// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== INICIO DEL SISTEMA =====\n");

  lv_init();
  lv_log_register_print_cb(log_print);

  spi_touch.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(spi_touch);
  touchscreen.setRotation(2);

  lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  style_init_tabs();
  lv_create_main_gui();

  if (!initSD()) return;

  Serial.println("\nSistema inicializado correctamente\n");
}

// ================== LOOP ==================
void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(5);
  checkSerialUpdate();
  checkSDStatus(); // <-- Revisión periódica de la SD
  updateSDDisplay();

  // Actualización de datos de barra superior
  float temp = getInternalTemp();
  //Serial.printf("Temperatura interna del CPU: %.2f C\n", temp);


  //hora = "12:46 PM";
  //fecha = "01/11/2025";
  //estadoInternet = "Conectado";

  lv_label_set_text(label_hora, hora.c_str());
  lv_label_set_text(label_fecha, fecha.c_str());
  lv_label_set_text(label_estado, estadoInternet.c_str());
}
