/*
C++ version for the ESP-IDF by Francois Blanchette.
https://github.com/cfrankb/esp32-tm1637plus-cpp

MicroPython TM1637 quad 7-segment LED display driver
https://github.com/mcauser/micropython-tm1637

MIT License
Copyright (c) 2016 Mike Causer
Copyright (c) 2024 Francois Blanchette

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "tm1637plus.h"
#include <esp_log.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string>
#include <cstring>
#include <math.h>
#include <vector>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if CONFIG_IDF_TARGET_ESP32
#include <esp32/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32S2
#include <esp32s2/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32S3
#include <esp32s3/rom/ets_sys.h>
#elif CONFIG_IDF_TARGET_ESP32C3
#include <esp32c3/rom/ets_sys.h>
#else
#error "Unsupported ESP chip"
#endif

#define TM1637_ADDR_AUTO 0x40
#define TM1637_ADDR_FIXED 0x44
#define TM1637_CMD1 64  //  # 0x40 data command
#define TM1637_CMD2 192 // # 0xC0 address command
#define TM1637_CMD3 128 // # 0x80 display control command
#define TM1637_DSP_ON 8 // # 0x08 display on
#define TM1637_DELAY 10 // # 10us delay between clk/dio pulses
#define TM1637_MSB 128  // # msb is the decimal point or the colon depending on your display

// 0-9, a-z, blank, dash, star
constexpr char _SEGMENTS[] = "\x3F\x06\x5B\x4F\x66\x6D\x7D\x07\x7F\x6F\x77\x7C\x39\x5E\x79\x71\x3D\x76\x06\x1E\x76\x38\x55\x54\x3F\x73\x67\x50\x6D\x78\x3E\x1C\x2A\x76\x6E\x5B\x00\x40\x63";
static const char *TAG = "tm1637";

TM1637::TM1637(gpio_num_t pin_clk, gpio_num_t pin_dta)
{
    ESP_LOGI(TAG, "init with clk: %d dta: %d brightness: %d", pin_clk, pin_dta, CONFIG_TM1637_BRIGHTNESS);
    m_pin_clk = pin_clk;
    m_pin_dta = pin_dta;
    m_brightness = CONFIG_TM1637_BRIGHTNESS & 7;

    // Set CLK to low during DIO initialization to avoid sending a start signal by mistake
    gpio_set_direction(pin_clk, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_clk, 0);
    gpio_set_direction(pin_dta, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_dta, 0);
    _delay();

    _write_data_cmd();
    _write_dsp_ctrl();
}

TM1637::~TM1637()
{
}

void TM1637::_delay()
{
    ets_delay_us(TM1637_DELAY);
}

void TM1637::_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; ++i)
    {
        gpio_set_level(m_pin_dta, (b >> i) & 1);
        _delay();
        gpio_set_level(m_pin_clk, 1);
        _delay();
        gpio_set_level(m_pin_clk, 0);
        _delay();
    }
    gpio_set_level(m_pin_clk, 0);
    _delay();
    gpio_set_level(m_pin_clk, 1);
    _delay();
    gpio_set_level(m_pin_clk, 0);
    _delay();
}

void TM1637::_start()
{
    gpio_set_level(m_pin_dta, 0);
    _delay();
    gpio_set_level(m_pin_clk, 0);
    _delay();
}

void TM1637::_stop()
{
    gpio_set_level(m_pin_dta, 0);
    _delay();
    gpio_set_level(m_pin_clk, 1);
    _delay();
    gpio_set_level(m_pin_dta, 1);
    _delay();
}

void TM1637::_write_data_cmd()
{
    _start();
    _write_byte(TM1637_CMD1);
    _stop();
}

void TM1637::_write_dsp_ctrl()
{
    _start();
    _write_byte(TM1637_CMD3 | TM1637_DSP_ON | m_brightness);
    _stop();
}

/// @brief Set the display brightness 0-7.
/// @param val brightness level
void TM1637::brightness(const uint8_t val)
{
    // # brightness 0 = 1/16th pulse width
    // # brightness 7 = 14/16th pulse width
    if (val > 7)
    {
        ESP_LOGW(TAG, "brightness: %d out of range.", val);
    }
    m_brightness = val & 7;
    _write_data_cmd();
    _write_dsp_ctrl();
}

/// @brief retrieve the current brightness level
/// @return current brightness level (0 - 7)
uint8_t TM1637::brightness()
{
    return m_brightness;
}

/// @brief write segment to the display at a given position
/// @param segments 7-digit segments
/// @param pos start position
void TM1637::write(std::vector<uint8_t> segments, int pos)
{
    // Display up to 6 segments moving right from a given position.
    // The MSB in the 2nd segment controls the colon between the 2nd
    // and 3rd segments.
    if (pos < 0 || pos > 4)
    {
        ESP_LOGE(TAG, "Position out of range:%d", pos);
        return;
    }
    _write_data_cmd();
    _start();

    _write_byte(TM1637_CMD2 | pos);
    for (int i = 0; i < segments.size(); ++i)
    {
        _write_byte(segments[i]);
    }
    _stop();
    _write_dsp_ctrl();
}

/// @brief Convert a character 0-9, a-f to a segment
/// @param digit value 0 to 9
/// @return 7gigit segment
uint8_t TM1637::encode_digit(const int digit)
{
    return _SEGMENTS[digit & 0x0f];
}

/// @brief Convert a character 0-9, a-z, space, dash or star to a segment.
/// @param o char / character
/// @return Convert a character 0-9, a-z, space, dash or star to a segment.
uint8_t TM1637::encode_char(const char o)
{
    if (o == ' ')
    {
        return _SEGMENTS[36];
    } // #space
    else if (o == 42)
    {
        return _SEGMENTS[38];
    } // # star/degrees
    else if (o == 45)
    {
        return _SEGMENTS[37];
    } // # dash
    else if (o >= 65 && o <= 90)
    {
        return _SEGMENTS[o - 55];
    } // # uppercase A-Z
    else if (o >= 97 and o <= 122)
    {
        return _SEGMENTS[o - 87];
    } // # lowercase a-z
    else if (o >= 48 and o <= 57)
    {
        return _SEGMENTS[o - 48];
    } // # 0-9
    else
    {
        ESP_LOGE(TAG, "Character out of range: %c", o);
        return _SEGMENTS[36];
    }
}

/// @brief encode the string into segments
/// @param s string
/// @return segments for the given string
const std::vector<uint8_t> &TM1637::encode_string(const char *s)
{
    static std::vector<uint8_t> segments;
    segments.clear();
    size_t size = std::min(strlen(s), static_cast<size_t>(4));
    for (size_t i = 0; i < size; ++i)
    {
        segments.emplace_back(encode_char(s[i]));
    }
    return segments;
}

/// @brief Display a hex value 0x0000 through 0xffff, right aligned.
/// @param val
/// @return hexadecimal representation for the value as 7digit segments
const std::vector<uint8_t> &TM1637::hex(uint16_t val)
{
    char s[10];
    sprintf(s, "%.4x", val);
    s[4] = 0;
    return encode_string(s);
}

/// @brief number
/// @param val
/// @return decimal representation of the input value as segments
const std::vector<uint8_t> &TM1637::number(uint16_t val)
{
    char s[10];
    sprintf(s, "%.4d", val);
    s[4] = 0;
    return encode_string(s);
}

/// @brief show a string on the display
/// @param s string
/// @param colon show colon (default false). Colon is not available on all displays.
void TM1637::show(const char *s, bool colon)
{
    std::vector<uint8_t> segments = encode_string(s);
    if (segments.size() > 1 && colon)
    {
        segments[1] |= 128;
    }
    write(segments);
}

/// @brief set the raw segment at a given position
/// @param pos desired position
/// @param data segment data
void TM1637::set_raw(const uint8_t pos, const uint8_t data)
{
    _start();
    _write_byte(TM1637_ADDR_FIXED);
    _stop();
    _start();
    _write_byte(pos | 0xc0);
    _write_byte(data);
    _stop();
    _start();
    _write_byte(m_brightness | 0x88);
    _stop();
}

/// @brief set a character at a given position
/// @param pos desired position
/// @param ch ascii char
/// @param dot enable dot at this position (default false)
void TM1637::set_char(const uint8_t pos, const char ch, const bool dot)
{
    uint8_t data = encode_char(ch);
    if (dot)
    {
        data |= 0x80; // Set DOT segment flag
    }
    set_raw(pos, data);
}

/// @brief Scroll text across the LED Displat
/// @param s string to display
/// @param delay scroll delay in milisecond
void TM1637::scroll(const char *s, int delay)
{
    std::string str = s;
    char buf[5];
    memset(buf, ' ', sizeof(buf));
    buf[4] = 0;
    for (int i = 0; i < str.length(); ++i)
    {
        vTaskDelay(delay / portTICK_PERIOD_MS);
        strcpy(buf, buf + 1);
        buf[3] = str[i];
        show(buf, false);
    }
}
