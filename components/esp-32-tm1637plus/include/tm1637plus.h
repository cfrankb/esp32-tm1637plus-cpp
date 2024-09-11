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


#ifndef __TM1637___H
#define __TM1637___H
#include <inttypes.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <vector>

class TM1637
{
public:
    explicit TM1637(gpio_num_t pin_clk, gpio_num_t pin_dta);
    ~TM1637();

    static uint8_t encode_digit(const int digit);
    static uint8_t encode_char(const char o);
    static const std::vector<uint8_t> &encode_string(const char *s);
    static const std::vector<uint8_t> &hex(uint16_t val);
    static const std::vector<uint8_t> &number(uint16_t val);
    void brightness(const uint8_t val);
    uint8_t brightness();
    void write(std::vector<uint8_t> segments, int pos = 0);
    void show(const char *s, bool colon = false);
    void set_raw(const uint8_t pos, const uint8_t data);
    void set_char(const uint8_t pos, const char ch, const bool dot);
    void scroll(const char *s, int delay = 250);

private:
    gpio_num_t m_pin_dta;
    gpio_num_t m_pin_clk;
    uint8_t m_brightness;

    void _delay();
    void _write_byte(uint8_t b);
    void _start();
    void _stop();
    void _write_data_cmd();
    void _write_dsp_ctrl();
};

#endif