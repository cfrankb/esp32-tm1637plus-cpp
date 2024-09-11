#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <time.h>
#include <sys/time.h>
#include "tm1637plus.h"
#include "sdkconfig.h"

/**
 * @file app_main.c
 * @brief Example application for the TM1637 LED segment display
 */

#define TAG "app_main"

// GPIO_NUM_18;
const gpio_num_t LED_CLK = static_cast<gpio_num_t>(CONFIG_TM1637_CLK_PIN);
// GPIO_NUM_19;
const gpio_num_t LED_DTA = static_cast<gpio_num_t>(CONFIG_TM1637_DIO_PIN);

void lcd_tm1637_demo(void *arg)
{
    TM1637 tm1637(LED_CLK, LED_DTA);

    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    while (true)
    {
        // Test brightness
        for (int x = 0; x < 7; x++)
        {
            tm1637.brightness(x);
            tm1637.show("8888", false);
            vTaskDelay(100);
        }

        // Test segment control
        uint8_t seg_data[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
        for (uint8_t x = 0; x < 32; ++x)
        {
            uint8_t data = seg_data[x % 6];
            tm1637.set_raw(0, data);
            tm1637.set_raw(1, data);
            tm1637.set_raw(2, data);
            tm1637.set_raw(3, data);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        for (uint8_t x = 0; x < 3; ++x)
        {
            // Set random system time
            struct timeval tm_test = {1517769863 + (x * 3456), 0};
            settimeofday(&tm_test, NULL);

            // Get current system time
            time_t now = 0;
            struct tm timeinfo = {0};
            time(&now);
            localtime_r(&now, &timeinfo);
            int time_number = 100 * timeinfo.tm_hour + timeinfo.tm_min;

            // Display time with blinking dots
            for (int z = 0; z < 5; ++z)
            {
                char tmp[5];
                sprintf(tmp, "%.4d", time_number);
                tm1637.set_char(0, tmp[0], z % 2);
                tm1637.set_char(1, tmp[1], z % 2);
                tm1637.set_char(2, tmp[2], z % 2);
                tm1637.set_char(3, tmp[3], z % 2);
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }

        // Test display numbers
        for (int x = 0; x < 16; ++x)
        {
            bool show_dot = x % 2; // Show dot every 2nd cycle
            uint8_t v = x < 10 ? x + '0' : x - 10 + 'a';
            tm1637.set_char(0, v, show_dot);
            tm1637.set_char(1, v, show_dot); // On my display "dot" (clock symbol ":") connected only here
            tm1637.set_char(2, v, show_dot);
            tm1637.set_char(3, v, show_dot);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // test scrolling
        tm1637.scroll("    getting the tm1637 to work in cpp was a lot fun   ");
    }
}

extern "C" void app_main(void)
{
    xTaskCreate(&lcd_tm1637_demo, "lcd_tm1637_task", 4096, NULL, 5, NULL);
}