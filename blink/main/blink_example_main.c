/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO
#define NUM_LEDS 25

static const char *TAG = "led_effects";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/

#ifdef CONFIG_BLINK_LED_STRIP
static led_strip_handle_t led_strip;

static void loop_effect(void)
{
    static size_t led_index = 0;

    while (1)
    {
        size_t prev_index = ((led_index + NUM_LEDS) - 1) % NUM_LEDS;
        size_t next_index = (led_index + 1) % NUM_LEDS;

        /* Turn off all leds */
        led_strip_clear(led_strip);

        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, prev_index, 0, 0, 15);
        led_strip_set_pixel(led_strip, next_index, 0, 0, 15);
        led_strip_set_pixel(led_strip, led_index, 0, 0, 50);

        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);

        led_index = (led_index + 1) % NUM_LEDS;
        vTaskDelay(CONFIG_BLINK_PERIOD / (portTICK_PERIOD_MS * 2));
    }
}

static void knight_rider_effect(void)
{
    static size_t led_index = 0;
    static int direction = 1;

    while (1)
    {
        led_strip_clear(led_strip);

        led_strip_set_pixel(led_strip, led_index, 50, 0, 0);
        if (led_index > 0)
        {
            led_strip_set_pixel(led_strip, led_index - 1, 20, 0, 0);
        }

        if (led_index > 1)
        {
            led_strip_set_pixel(led_strip, led_index - 2, 5, 0, 0);
        }

        if (led_index < (NUM_LEDS - 1))
        {
            led_strip_set_pixel(led_strip, led_index + 1, 20, 0, 0);
        }

        if (led_index < (NUM_LEDS - 2))
        {
            led_strip_set_pixel(led_strip, led_index + 2, 5, 0, 0);
        }

        led_strip_refresh(led_strip);

        led_index += direction;
        if ((led_index == NUM_LEDS - 1) || (led_index == 0))
        {
            direction = -direction;
        }

        vTaskDelay(CONFIG_BLINK_PERIOD / (portTICK_PERIOD_MS * 4));
    }
}

// Helper: HSV (Hue, Saturation, Value) to RGB conversion
static void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1, g1, b1;

    if (h < 60)
    {
        r1 = c;
        g1 = x;
        b1 = 0;
    }
    else if (h < 120)
    {
        r1 = x;
        g1 = c;
        b1 = 0;
    }
    else if (h < 180)
    {
        r1 = 0;
        g1 = c;
        b1 = x;
    }
    else if (h < 240)
    {
        r1 = 0;
        g1 = x;
        b1 = c;
    }
    else if (h < 300)
    {
        r1 = x;
        g1 = 0;
        b1 = c;
    }
    else
    {
        r1 = c;
        g1 = 0;
        b1 = x;
    }

    *r = (r1 + m) * 255;
    *g = (g1 + m) * 255;
    *b = (b1 + m) * 255;
}

static void rainbow_effect(void)
{
    static float hue_offset = 0.0f;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Spread hues evenly along the strip and shift them over time
        float hue = fmodf((i * 360.0f / NUM_LEDS) + hue_offset, 360.0f);

        uint8_t r, g, b;
        hsv_to_rgb(hue, 1.0f, 0.3f, &r, &g, &b); // s=1.0 full color, v=0.3 brightness
        led_strip_set_pixel(led_strip, i, r, g, b);
    }

    led_strip_refresh(led_strip);

    // Slowly move the rainbow
    hue_offset = fmodf(hue_offset + 3.0f, 360.0f);
    vTaskDelay(CONFIG_BLINK_PERIOD / (portTICK_PERIOD_MS * 2));
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = NUM_LEDS, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    int mode = 2;

    ESP_LOGI(TAG, "Starting effect %d", mode);

    switch (mode)
    {
    case 0:
        loop_effect();
        break;
    case 1:
        knight_rider_effect();
        break;
    case 2:
        rainbow_effect();
        break;
    }
}
