#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "Fuzzy.h"
//#include "esp_dsp.h"



static const gpio_num_t led1 = GPIO_NUM_2;  // ✅ Tipo correcto para C++

uint8_t led_level = 0;

esp_err_t init_led(void);
esp_err_t blink_led(void);

extern "C" void app_main(void) {  // ✅ En C++, app_main debe ir dentro de extern "C"
    init_led();
    while (1) {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        blink_led();
        printf("Led Level %u\n", led_level);
    }
}

esp_err_t init_led(void) {
    gpio_reset_pin(led1);
    gpio_set_direction(led1, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

esp_err_t blink_led(void) {
    led_level = !led_level;
    gpio_set_level(led1, led_level);
    return ESP_OK;
}
